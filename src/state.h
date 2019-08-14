#ifndef _STATE_H_
#define _STATE_H_

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

const uint8_t kClockStopped = UINT8_MAX;

static inline void patch_init(state_t* s) {
    for (uint8_t row = 0; row < kNumRows; row++) {
        for (uint8_t step = 0; step < kNumSteps; step++) {
            s->patch.rows[row].step[step] = false;
        }
    }
}

static inline void state_init(state_t* s) {
    s->clock = kClockStopped;
    s->ui_dirty = true;
    patch_init(s);
}

static inline uint8_t state_clock(state_t* s) {
    return s->clock;
}

static inline void state_clock_reset(state_t* s) {
    s->clock = kClockStopped;
}

static inline void state_tick(state_t* s) {
    s->clock = (s->clock + 1) % kNumSteps;
}

static inline void state_ui_dirty(state_t* s) {
    s->ui_dirty = true;
}

static inline void state_ui_clean(state_t* s) {
    s->ui_dirty = false;
}

static inline bool state_is_ui_dirty(state_t* s) {
    return s->ui_dirty;
}

static inline void patch_toggle_step(state_t* s, uint8_t row, uint8_t step) {
    if (row > kNumRows || step > kNumSteps) return;
    s->patch.rows[row].step[step] = !s->patch.rows[row].step[step];
}

static inline bool patch_step_value(state_t* s, uint8_t row, uint8_t step) {
    if (row > kNumRows || step > kNumSteps) return false;
    return s->patch.rows[row].step[step];
}


#endif
