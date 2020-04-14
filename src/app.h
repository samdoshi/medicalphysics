#ifndef _APP_H_
#define _APP_H_

#include <stdbool.h>

#include "state.h"

void app_clock(state_t *state, bool phase);
void app_reset(state_t *state);
void app_refresh(state_t *state);

#endif
