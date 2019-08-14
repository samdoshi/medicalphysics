#ifndef _INIT_MEADOWPHYSICS_H_
#define _INIT_MEADOWPHYSICS_H_

#include "types.h"

typedef void (*clock_pulse_t)(uint8_t phase);
extern volatile clock_pulse_t clock_pulse;

extern void register_interrupts(void);
extern void init_gpio(void);
extern void init_spi(void);

extern uint64_t get_ticks(void);

#endif
