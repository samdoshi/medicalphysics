#include "app.h"

#include "hardware.h"


void app_reset(state_t *s) {
    state_clock_reset(s);
    state_ui_dirty(s);
}

void app_clock(state_t *s, bool phase) {
    if (phase) {
        state_tick(s);
        state_ui_dirty(s);

        hardware_set_clock_output(true);

        uint8_t step = state_clock(s);
        for (uint8_t row = 0; row < kNumRows; row++) {
            if (patch_step_value(s, row, step)) {
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

void app_grid_press(state_t *s, uint8_t x, uint8_t y, uint8_t z) {
    // bail on key up
    if (z == 0) return;

    patch_toggle_step(s, y, x);
    state_ui_dirty(s);
}

void app_refresh(state_t *s) {
    const uint8_t kCheckerLed = 2;
    const uint8_t kClockLed = 6;
    const uint8_t kTriggerLed = 10;
    const uint8_t kTriggerClockLed = 15;

    grid_arc_clear();

    uint8_t c = state_clock(s);
    for (uint8_t row = 0; row < kNumRows; row++) {
        for (uint8_t step = 0; step < kNumSteps; step++) {
            if (patch_step_value(s, row, step)) {
                if (step == c)
                    grid_set(step, row, kTriggerClockLed);
                else
                    grid_set(step, row, kTriggerLed);
            }
            else if (step == c) {
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
    state_ui_clean(s);
}
