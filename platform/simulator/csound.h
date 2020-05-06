#ifndef _CSOUND_H_
#define _CSOUND_H_

#include <stdbool.h>
#include <stdint.h>

void start_csound(void);
void stop_csound(void);
void csound_set_trigger_output(uint8_t idx, bool state);

#endif
