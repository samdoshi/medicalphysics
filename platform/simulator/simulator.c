#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <csound/csound.h>
#include <monome.h>

#include "app.h"
#include "hardware.h"
#include "orchestras.h"
#include "state.h"
#include "timers.h"

// flag to indicate that the simulator should quit (to help Csound quit)
bool quit_now = false;

// hardware has no concept of playing notes like Csound does,
// this is used to compensate for that
bool trigger_playing[8] = { false };

// Csound thread
typedef struct {
    CSOUND *csound;
} cs_user_data_t;
cs_user_data_t cs_user_data = {};

uintptr_t cs_thread(void *data) {
    cs_user_data_t *ud = (cs_user_data_t *)data;
    while ((csoundPerformKsmps(ud->csound) == 0) && (quit_now == false)) {
    }
    csoundDestroy(ud->csound);
    return 1;
}

// grid status
#define GRID_SIZE 16
uint8_t grid[GRID_SIZE][GRID_SIZE] = { { 0 } };

#define QUADRANTS 4
bool quadrant_dirty[QUADRANTS] = { false };

monome_t *monome;
state_t state;

void hardware_set_clock_output(bool val) {
    if (!val) printf("\n");
}

void hardware_set_trigger_output(uint8_t idx, bool val) {
    const char *i[] = { "1", "2", "3", "4", "5", "6", "7", "8" };
    const char *const n[] = { "9.00", "9.02", "9.04", "9.05",
                              "9.07", "9.09", "9.11", "10.00" };
    if (val) {
        printf("T%d", idx);
        // "i 1.1 0 -1 9.04"
        char m[255] = "i 1.";
        strcat(m, i[idx]);
        strcat(m, " 0 -1 ");
        strcat(m, n[idx]);
        csoundInputMessageAsync(cs_user_data.csound, m);
        trigger_playing[idx] = true;
    }
    else if (trigger_playing[idx] == true) {
        // "i -1.1 0 0"
        char m[255] = "i -1.";
        strcat(m, i[idx]);
        strcat(m, " 0 0");
        csoundInputMessageAsync(cs_user_data.csound, m);
        trigger_playing[idx] = false;
    }
}

void grid_set_dirty(uint8_t quadrant) {
    if (quadrant >= QUADRANTS) return;
    quadrant_dirty[quadrant] = true;
}

void grid_arc_clear(void) {
    memset(grid, 0, GRID_SIZE * GRID_SIZE);
}

void grid_set(uint8_t x, uint8_t y, uint8_t level) {
    if (x >= GRID_SIZE || y >= GRID_SIZE) return;
    grid[y][x] = level;
}

void grid_refresh() {
    for (uint8_t q = 0; q < 4; q++) {
        if (quadrant_dirty[q] == false) continue;

        uint8_t off_x = 0;
        uint8_t off_y = 0;
        switch (q) {
            case 0:
                off_x = 0;
                off_y = 0;
                break;
            case 1:
                off_x = 8;
                off_y = 0;
                break;
            case 2:
                off_x = 8;
                off_y = 0;
                break;
            case 3:
                off_x = 8;
                off_y = 8;
                break;
        }
        uint8_t data[64];
        for (uint8_t y = 0; y < 8; y++) {
            for (uint8_t x = 0; x < 8; x++) {
                data[y * 8 + x] = grid[y + off_y][x + off_x];
            }
        }
        monome_led_level_map(monome, off_x, off_y, data);
        quadrant_dirty[q] = false;
    }
}

void handle_press(const monome_event_t *e, void *user_data) {
    uint8_t x = (uint8_t)e->grid.x;
    uint8_t y = (uint8_t)e->grid.y;
    uint8_t z = (uint8_t)user_data;
    app_grid_press(&state, x, y, z);
}

void handle_clock() {
    static bool clock = false;  // start clock low
    app_clock(&state, clock);
    clock = !clock;
}

void handle_refresh() {
    if (state_is_ui_dirty(&state)) app_refresh(&state);
}

void set_quit_now() {
    quit_now = true;
}

void no_message_callback(CSOUND *cs, int attr, const char *format,
                         va_list valist) {
    // Do nothing so that Csound will not print any message,
    // leaving a clean console for our app
    return;
}

int main() {
    monome = monome_open("/dev/ttyUSB0");
    if (!monome) {
        printf("Connection failed\n");
        return -1;
    }

    // Ctrl-C handler
    signal(SIGINT, set_quit_now);

    // Csound set up
    // silence Csound messages
    csoundSetDefaultMessageCallback(no_message_callback);
    csoundInitialize(CSOUNDINIT_NO_ATEXIT | CSOUNDINIT_NO_SIGNAL_HANDLER);
    cs_user_data.csound = csoundCreate(NULL);

    csoundSetOption(cs_user_data.csound, "-odac");
    csoundSetOption(cs_user_data.csound, "-d");
    if (csoundCompileOrc(cs_user_data.csound, simple_trigger_orc) != 0) {
        printf("Orchestra compile failed\n");
        return -1;
    }
    csoundStart(cs_user_data.csound);


    void *cs_thread_id = csoundCreateThread(cs_thread, &cs_user_data);

    monome_register_handler(monome, MONOME_BUTTON_DOWN, handle_press,
                            (void *)0);
    monome_register_handler(monome, MONOME_BUTTON_UP, handle_press, (void *)1);

    setbuf(stdout, NULL);

    set_clock_callback(handle_clock);
    set_clock_rate(120.0 * 8);
    set_refresh_callback(handle_refresh);

    while (!quit_now) {
        while (monome_event_handle_next(monome)) {
        }
        struct timespec next = process_timers();
        sleep_till_before(next);
    }

    monome_led_all(monome, 0);
    monome_close(monome);

    csoundJoinThread(cs_thread_id);
    csoundDestroy(cs_user_data.csound);

    return 0;
};
