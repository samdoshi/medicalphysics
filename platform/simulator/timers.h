#ifndef _TIMERS_H_
#define _TIMERS_H_

#include <time.h>


void set_clock_callback(void (*callback)());
void set_refresh_callback(void (*callback)());

void set_clock_rate(double bpm);
struct timespec process_timers(void);
void sleep_till_before(struct timespec when);

#endif
