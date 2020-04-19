#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#include <stdbool.h>
#include <stdint.h>

void hardware_set_clock_output(bool val);
void hardware_set_trigger_output(uint8_t idx, bool val);

void grid_set_dirty(uint8_t quadrant);
void grid_arc_clear(void);
void grid_set(uint8_t x, uint8_t y, uint8_t level);
void grid_refresh(void);

#endif
