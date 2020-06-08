#include "app.h"

#include "hardware.h"

#define kClockStopped UINT8_MAX;

static void patch_init(state_t* s) {
    for (uint8_t row = 0; row < kNumRows; row++) {
        for (uint8_t step = 0; step < kNumSteps; step++) {
            s->patch.rows[row].step[step] = false;
        }
    }
}

static void patch_toggle_step(state_t* s, uint8_t row, uint8_t step) {
    if (row > kNumRows || step > kNumSteps) return;
    s->patch.rows[row].step[step] = !s->patch.rows[row].step[step];
}

static bool patch_step_value(state_t* s, uint8_t row, uint8_t step) {
    if (row > kNumRows || step > kNumSteps) return false;
    return s->patch.rows[row].step[step];
}

void app_init(state_t* s) {
    s->clock = kClockStopped;
    s->ui_dirty = true;
    patch_init(s);
}

void app_reset(state_t* s) {
    s->clock = kClockStopped;
    s->ui_dirty = true;
}

void app_clock(state_t* s, bool phase) {
    if (phase) {
        s->clock = (s->clock + 1) % kNumSteps;  // advance clock (or reset to 0)
        s->ui_dirty = true;

        hardware_set_clock_output(true);

        for (uint8_t row = 0; row < kNumRows; row++) {
            if (patch_step_value(s, row, s->clock)) {
                hardware_set_trigger_output(row, true);
            }
            else {
                hardware_set_trigger_output(row, false);
            }
        }
    }
    else {
        hardware_set_clock_output(false);
        for (uint8_t row = 0; row < kNumRows; row++) {
            hardware_set_trigger_output(row, false);
        }
    }
}

void app_grid_press(state_t* s, uint8_t x, uint8_t y, uint8_t z) {
    // bail on key up
    if (z == 0) return;

    patch_toggle_step(s, y, x);
    s->ui_dirty = true;
}

void app_refresh(state_t* s) {
    const uint8_t kCheckerLed = 2;
    const uint8_t kClockLed = 6;
    const uint8_t kTriggerLed = 10;
    const uint8_t kTriggerClockLed = 15;

    grid_arc_clear();

    for (uint8_t row = 0; row < kNumRows; row++) {
        for (uint8_t step = 0; step < kNumSteps; step++) {
            if (patch_step_value(s, row, step)) {
                if (step == s->clock)
                    grid_set(step, row, kTriggerClockLed);
                else
                    grid_set(step, row, kTriggerLed);
            }
            else if (step == s->clock) {
                grid_set(step, row, kClockLed);
            }
            else {
                // draw checker board
                if (((row / 4) % 2) && ((step / 4) % 2)) {
                    grid_set(step, row, kCheckerLed);
                }
                if (!((row / 4) % 2) && !((step / 4) % 2)) {
                    grid_set(step, row, kCheckerLed);
                }
            }
        }
    }

    // set the grid as being dirty
    grid_set_dirty(0);
    grid_set_dirty(1);
    // do the refresh
    grid_refresh();
    // mark the ui as clean
    s->ui_dirty = false;
}

bool app_grid_is_dirty(state_t* s) {
    return s->ui_dirty;
}
