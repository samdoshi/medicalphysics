// ASF
#include <sysclk.h>

#include "gpio.h"
#include "intc.h"
#include "print_funcs.h"
#include "spi.h"
#include "tc.h"
#include "usart.h"

#include "conf_board.h"
#include "conf_tc_irq.h"
#include "events.h"
#include "timers.h"
#include "types.h"

#include "init_meadowphysics.h"


// timer tick counter
static volatile uint64_t tcTicks = 0;

static void clock_null(uint8_t phase) {}
volatile clock_pulse_t clock_pulse = &clock_null;

// interrupt handlers

// irq for app timer
__attribute__((__interrupt__)) static void irq_tc(void);

// irq for PA08-PA15
__attribute__((__interrupt__)) static void irq_port0_line1(void);

// irq for PB08-PB15
__attribute__((__interrupt__)) static void irq_port1_line1(void);

// timer irq
__attribute__((__interrupt__)) static void irq_tc(void) {
    tcTicks++;
    process_timers();
    // clear interrupt flag by reading timer SR
    tc_read_sr(APP_TC, APP_TC_CHANNEL);
}

// interrupt handler for PA08-PA15
__attribute__((__interrupt__)) static void irq_port0_line1(void) {
    if (gpio_get_pin_interrupt_flag(NMI)) {
        gpio_clear_pin_interrupt_flag(NMI);
        event_t e = { .type = kEventFront, .data = !gpio_get_pin_value(NMI) };
        event_post(&e);
    }
}

// interrupt handler for PB08-PB15
__attribute__((__interrupt__)) static void irq_port1_line1(void) {
    // clock norm
    if (gpio_get_pin_interrupt_flag(B09)) {
        event_t e = { .type = kEventClockNormal,
                      .data = !gpio_get_pin_value(B09) };
        event_post(&e);

        gpio_clear_pin_interrupt_flag(B09);
    }

    // clock in
    if (gpio_get_pin_interrupt_flag(B08)) {
        event_t e = { .type = kEventClockExt, .data = gpio_get_pin_value(B08) };
        event_post(&e);
        gpio_clear_pin_interrupt_flag(B08);
    }
}

// register interrupts
void register_interrupts(void) {
    // enable interrupts on GPIO inputs
    gpio_enable_pin_interrupt(NMI, GPIO_PIN_CHANGE);
    gpio_enable_pin_interrupt(B08, GPIO_PIN_CHANGE);
    gpio_enable_pin_interrupt(B09, GPIO_PIN_CHANGE);

    // PA08 - PA15
    INTC_register_interrupt(&irq_port0_line1,
                            AVR32_GPIO_IRQ_0 + (AVR32_PIN_PA08 / 8),
                            UI_IRQ_PRIORITY);

    // PB08 - PB15
    INTC_register_interrupt(&irq_port1_line1,
                            AVR32_GPIO_IRQ_0 + (AVR32_PIN_PB08 / 8),
                            UI_IRQ_PRIORITY);

    // register TC interrupt
    INTC_register_interrupt(&irq_tc, APP_TC_IRQ, UI_IRQ_PRIORITY);
}

extern void init_gpio(void) {
    gpio_enable_gpio_pin(B00);
    gpio_enable_gpio_pin(B01);
    gpio_enable_gpio_pin(B02);
    gpio_enable_gpio_pin(B03);
    gpio_enable_gpio_pin(B04);
    gpio_enable_gpio_pin(B05);
    gpio_enable_gpio_pin(B06);
    gpio_enable_gpio_pin(B07);
    gpio_enable_gpio_pin(B08);
    gpio_enable_gpio_pin(B09);
    gpio_enable_gpio_pin(B10);
    gpio_enable_gpio_pin(NMI);
}


extern void init_spi(void) {
    sysclk_enable_pba_module(SYSCLK_SPI);

    static const gpio_map_t SPI_GPIO_MAP = {
        { SPI_SCK_PIN, SPI_SCK_FUNCTION },
        { SPI_MISO_PIN, SPI_MISO_FUNCTION },
        { SPI_MOSI_PIN, SPI_MOSI_FUNCTION },
        { SPI_NPCS0_PIN, SPI_NPCS0_FUNCTION },
        { SPI_NPCS1_PIN, SPI_NPCS1_FUNCTION },
    };

    // Assign GPIO to SPI.
    gpio_enable_module(SPI_GPIO_MAP,
                       sizeof(SPI_GPIO_MAP) / sizeof(SPI_GPIO_MAP[0]));

    spi_options_t spiOptions = { .reg = DAC_SPI_NPCS,
                                 .baudrate = 2000000,
                                 .bits = 8,
                                 .trans_delay = 0,
                                 .spck_delay = 0,
                                 .stay_act = 1,
                                 .spi_mode = 1,
                                 .modfdis = 1 };


    // Initialize as master.
    spi_initMaster(SPI, &spiOptions);
    // Set SPI selection mode: variable_ps, pcs_decode, delay.
    spi_selectionMode(SPI, 0, 0, 0);
    // Enable SPI module.
    spi_enable(SPI);

    // spi_setupChipReg( SPI, &spiOptions, FPBA_HZ );
    spi_setupChipReg(SPI, &spiOptions, sysclk_get_pba_hz());

    // add ADC chip register
    spiOptions.reg = ADC_SPI_NPCS;
    spiOptions.baudrate = 20000000;
    spiOptions.bits = 16;
    spiOptions.spi_mode = 2;
    spiOptions.spck_delay = 0;
    spiOptions.trans_delay = 5;
    spiOptions.stay_act = 0;
    spiOptions.modfdis = 0;

    spi_setupChipReg(SPI, &spiOptions, FPBA_HZ);
}

extern uint64_t get_ticks(void) {
    return tcTicks;
}
