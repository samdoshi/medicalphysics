#include <stdio.h>
#include <string.h>

// asf
#include "avr32_reset_cause.h"
#include "compiler.h"
#include "delay.h"
#include "flashc.h"
#include "gpio.h"
#include "intc.h"
#include "pm.h"
#include "preprocessor.h"
#include "print_funcs.h"
#include "spi.h"
#include "sysclk.h"

// libavr32
#include "types.h"

#include "adc.h"
#include "events.h"
#include "ftdi.h"
#include "i2c.h"
#include "ii.h"
#include "init_common.h"
#include "monome.h"
#include "timers.h"
#include "twi.h"
#include "util.h"

// this
#include "conf_board.h"

#include "init_meadowphysics.h"

#include "app.h"
#include "hardware.h"
#include "state.h"

#define kNumOutputs 8
const uint32_t kOutputs[kNumOutputs] = {
    B00, B01, B02, B03, B04, B05, B06, B07
};

const uint32_t kClockNormal = B09;
const uint32_t kClockOut = B10;

#define kNumClockTracking 8

typedef struct {
    uint16_t front_held_timer;
    bool clock_phase;
    uint16_t clock_time;
    uint16_t clock_prev;
    bool clock_external;
    intptr_t clock_tracking_idx;
    int32_t clock_tracking[kNumClockTracking];
    uint32_t last_sys_count;
} hardware_state_t;

// hardware.h

// 'static inline' in a header file?
void hardware_set_trigger_output(uint8_t idx, bool val) {
    if (idx >= kNumOutputs) return;

    uint32_t pin = kOutputs[idx];

    if (val) {
        gpio_set_gpio_pin(pin);
    }
    else {
        gpio_clr_gpio_pin(pin);
    }
}

// 'static inline' in a header file?
void hardware_set_clock_output(bool val) {
    if (val) {
        gpio_set_gpio_pin(kClockOut);
    }
    else {
        gpio_clr_gpio_pin(kClockOut);
    }
}

void grid_set_dirty(void) {
    monome_set_quadrant_flag(0);
    monome_set_quadrant_flag(1);
}

void grid_arc_clear(void) {
    memset(monomeLedBuffer, 0, MONOME_MAX_LED_BYTES);
}

void grid_set(uint8_t x, uint8_t y, uint8_t level) {
    monome_led_set(x, y, level);
}

void grid_refresh() {
    (*monome_refresh)();
}

// state

static hardware_state_t hw_state = {
    .front_held_timer = 0,
    .clock_prev = UINT16_MAX  // out of ADC range to force tempo
};

static state_t state;

typedef const struct {
    uint8_t fresh;  // this will hold 0xFF when it hasn't been used yet
    patch_t patch;
} nvram_data_t;

// NVRAM data structure located in the flash array.
__attribute__((__section__(".flash_nvram"))) static nvram_data_t flash;

static bool flash_empty(void);
static void flash_read(state_t* s);
static void flash_write(state_t* s);

////////////////////////////////////////////////////////////////////////////////
// clock tracking
//
// The AVR32_COUNT register (used by Get_sys_count) will overflow approx. every
// 71 seconds.

static void debug_clock_tracking(void) {
    print_dbg("\r\n");
    for (intptr_t i = 0; i < kNumClockTracking; i++) {
        int32_t v = hw_state.clock_tracking[i];
        if (v == -1) {
            print_dbg("X");
        }
        else {
            print_dbg_ulong(v);
        }
        if (i < (kNumClockTracking - 1)) print_dbg(", ");
    }
}

static void empty_clock_tracking(void) {
    for (intptr_t i = 0; i < kNumClockTracking; i++) {
        hw_state.clock_tracking[i] = -1;
    }
    hw_state.clock_tracking_idx = 0;
}

static void save_clock_tracking(void) {
    uint32_t sc = Get_sys_count();
    uint32_t lsc = hw_state.last_sys_count;

    int32_t last = hw_state.clock_tracking[hw_state.clock_tracking_idx];

    bool overflow = false;

    if (last != -1) {  // only increment idx if there is a value
        hw_state.clock_tracking_idx++;
        if (hw_state.clock_tracking_idx >= kNumClockTracking) {
            hw_state.clock_tracking_idx = 0;
        }
    }

    if (sc > lsc) {
        hw_state.clock_tracking[hw_state.clock_tracking_idx] = sc - lsc;
    }
    else if (sc < lsc) {  // overflow
        overflow = true;
        int32_t v = (UINT32_MAX - lsc) + sc;
        hw_state.clock_tracking[hw_state.clock_tracking_idx] = v;
    }

    hw_state.last_sys_count = sc;

    if (overflow) {
        print_dbg("\r\nOVERFLOW:");
        debug_clock_tracking();
    }
    // print_dbg("\r\n\r\nlsc: ");
    // print_dbg_ulong(lsc);
    // print_dbg("\r\nsc: ");
    // print_dbg_ulong(sc);
    // print_dbg("\r\nsc - lsc: ");
    // print_dbg_ulong(sc - lsc);
}

////////////////////////////////////////////////////////////////////////////////
// application code


////////////////////////////////////////////////////////////////////////////////
// timers

// used by the internal clock generator when no external clock is used
static softTimer_t clockTimer = { .next = NULL, .prev = NULL };

// used to detect long presses
static softTimer_t keyTimer = { .next = NULL, .prev = NULL };

// used to poll the ADC (i.e. clock knob)
static softTimer_t adcTimer = { .next = NULL, .prev = NULL };

// how often to poll for data from your grid/arc
static softTimer_t monomePollTimer = { .next = NULL, .prev = NULL };

// how often to ask the app if it needs to update the grid/arc state
static softTimer_t monomeRefreshTimer = { .next = NULL, .prev = NULL };


static void clockTimer_callback(void* o) {
    if (!hw_state.clock_external) {
        hw_state.clock_phase = !hw_state.clock_phase;
        app_clock(&state, hw_state.clock_phase);
    }
}

static void keyTimer_callback(void* o) {
    event_t e = { .type = kEventKeyTimer, .data = 0 };
    event_post(&e);
}

static void adcTimer_callback(void* o) {
    event_t e = { .type = kEventPollADC, .data = 0 };
    event_post(&e);
}

static void monome_poll_timer_callback(void* obj) {
    // asynchronous, non-blocking read
    // UHC callback spawns appropriate events
    ftdi_read();
}

static void monome_refresh_timer_callback(void* obj) {
    if (state_is_ui_dirty(&state)) {
        event_t e = { .type = kEventMonomeRefresh, e.data = 0 };
        event_post(&e);
    }
}


////////////////////////////////////////////////////////////////////////////////
// event handlers
static void handler_None(int32_t data) {}

static void handler_FtdiConnect(int32_t data) {
    ftdi_setup();
}
static void handler_FtdiDisconnect(int32_t data) {
    timer_remove(&monomePollTimer);
    timer_remove(&monomeRefreshTimer);
}

static void handler_MonomeConnect(int32_t data) {
    timer_add(&monomePollTimer, 20, &monome_poll_timer_callback, NULL);
    timer_add(&monomeRefreshTimer, 30, &monome_refresh_timer_callback, NULL);
}

static void handler_MonomePoll(int32_t data) {
    monome_read_serial();
}
static void handler_MonomeRefresh(int32_t data) {
    app_refresh(&state);
}

static void handler_PollADC(int32_t data) {
    uint16_t adc[4];
    adc_convert(&adc);

    // CLOCK POT INPUT
    uint16_t i = adc[0] >> 2;
    if (i != hw_state.clock_prev) {
        // 500ms - 12ms
        hw_state.clock_time = 12500 / (i + 25);
        timer_set(&clockTimer, hw_state.clock_time);
    }
    hw_state.clock_prev = i;
}

static void handler_Front(int32_t data) {
    if (data) {  // button down
        hw_state.front_held_timer = 1;
    }
    else {  // button up
        if (hw_state.front_held_timer < 15) {
            event_t e = { .type = kEventFrontShort, .data = 0 };
            event_post(&e);
        }
        else {
            event_t e = { .type = kEventFrontLong, .data = 0 };
            event_post(&e);
        }
        hw_state.front_held_timer = 0;
    }
}

static void handler_KeyTimer(int32_t data) {
    if (hw_state.front_held_timer >= 1) {
        hw_state.front_held_timer++;
    }

    if (hw_state.front_held_timer > 150) {
        reset_do_soft_reset();
    }
}

static void handler_FrontShort(int32_t data) {
    debug_clock_tracking();
    app_reset(&state);
}

static void handler_FrontLong(int32_t data) {
    flash_write(&state);
}

static void handler_ClockNormal(int32_t data) {
    hw_state.clock_external = !gpio_get_pin_value(kClockNormal);
}

static void handler_ClockExt(int32_t data) {
    if (data) {  // only on clock high
        // TODO: this should be called directly from the interrupt handler
        save_clock_tracking();
    }
    app_clock(&state, data);
}

static void handler_MonomeGridKey(int32_t data) {
    uint8_t x, y, z;
    monome_grid_key_parse_event_data(data, &x, &y, &z);

    // bail on key up
    if (z == 0) return;

    patch_toggle_step(&state, y, x);
    state_ui_dirty(&state);
}

static void mp_process_ii(uint8_t* d, uint8_t len) {}

// flash
static bool flash_empty() {
    return flash.fresh == 0xFF;
}

static void flash_read(state_t* s) {
    if (!flash_empty()) {
        memcpy(&s->patch, &flash.patch, sizeof(s->patch));
    }
}

static void flash_write(state_t* s) {
    flashc_memcpy((void*)&flash.patch, &s->patch, sizeof(s->patch), true);

    if (flash_empty()) {
        flashc_memset8((void*)&(flash.fresh), 0x00, 1, true);
    }
}


// assign event handlers
static void assign_main_event_handlers(void) {
    app_event_handlers[kEventFront] = &handler_Front;
    app_event_handlers[kEventFrontShort] = &handler_FrontShort;
    app_event_handlers[kEventFrontLong] = &handler_FrontLong;
    app_event_handlers[kEventKeyTimer] = &handler_KeyTimer;
    app_event_handlers[kEventPollADC] = &handler_PollADC;
    app_event_handlers[kEventClockNormal] = &handler_ClockNormal;
    app_event_handlers[kEventClockExt] = &handler_ClockExt;
    app_event_handlers[kEventFtdiConnect] = &handler_FtdiConnect;
    app_event_handlers[kEventFtdiDisconnect] = &handler_FtdiDisconnect;
    app_event_handlers[kEventMonomeConnect] = &handler_MonomeConnect;
    app_event_handlers[kEventMonomeDisconnect] = &handler_None;
    app_event_handlers[kEventMonomePoll] = &handler_MonomePoll;
    app_event_handlers[kEventMonomeRefresh] = &handler_MonomeRefresh;
    app_event_handlers[kEventMonomeGridKey] = &handler_MonomeGridKey;
}


// app event loop
static void check_events(void) {
    event_t e;
    if (event_next(&e)) {
        (app_event_handlers)[e.type](e.data);
    }
}


// main
int main(void) {
    sysclk_init();

    init_dbg_rs232(FMCK_HZ);

    init_gpio();

    // clear all gpio
    for (uint8_t i = 0; i < kNumOutputs; i++) {
        gpio_clr_gpio_pin(kOutputs[i]);
    }
    gpio_clr_gpio_pin(kClockOut);

    assign_main_event_handlers();
    init_events();
    init_tc();
    init_spi();
    init_adc();

    irq_initialize_vectors();
    register_interrupts();
    cpu_irq_enable();

    init_usb_host();
    init_monome();

    init_i2c_slave(0x41);

    // initialisation complete
    print_dbg("\r\nMedical Physics\r\n");

    empty_clock_tracking();
    hw_state.clock_external = !gpio_get_pin_value(kClockNormal);

    state_init(&state);
    flash_read(&state);

    timer_add(&clockTimer, 120, &clockTimer_callback, NULL);
    timer_add(&keyTimer, 50, &keyTimer_callback, NULL);
    timer_add(&adcTimer, 100, &adcTimer_callback, NULL);

    process_ii = &mp_process_ii;

    while (true) {
        check_events();
    }
}
