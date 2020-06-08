#ifndef _APP_H_
#define _APP_H_

#include <stdbool.h>
#include <stdint.h>

#define kNumSteps 16
typedef struct {
    bool step[kNumSteps];
} row_t;

#define kNumRows 8
typedef struct {
    row_t rows[kNumRows];
} patch_t;

typedef struct {
    // only 16 steps...
    uint8_t clock;
    // is the UI dirty? (i.e. does the grid need redrawing)
    bool ui_dirty;
    patch_t patch;
} state_t;

void app_init(state_t *state);
void app_clock(state_t *state, bool phase);
void app_grid_press(state_t *state, uint8_t x, uint8_t y, uint8_t z);
void app_reset(state_t *state);
void app_refresh(state_t *state);
bool app_grid_is_dirty(state_t *state);

#endif
