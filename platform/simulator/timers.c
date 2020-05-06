#include "timers.h"

#include <time.h>

#include "timespec.h"

#define NSEC_PER_MS 1000000
#define NUM_TIMERS 2

typedef struct {
    struct timespec every;
    struct timespec next;
    void (*callback)();
} event_timer_t;

const struct timespec kNullTimeSpec = { .tv_sec = 0, .tv_nsec = 0 };

event_timer_t clock_timer = { .every = { .tv_sec = 0,
                                         .tv_nsec = 50 * NSEC_PER_MS },
                              .next = { .tv_sec = 0, .tv_nsec = 0 },
                              .callback = NULL };

event_timer_t refresh_timer = { .every = { .tv_sec = 0,
                                           .tv_nsec = 50 * NSEC_PER_MS },
                                .next = { .tv_sec = 0, .tv_nsec = 0 },
                                .callback = NULL };


event_timer_t *const timers[NUM_TIMERS] = { &clock_timer, &refresh_timer };

void set_clock_callback(void (*callback)()) {
    clock_timer.callback = callback;
}

void set_refresh_callback(void (*callback)()) {
    refresh_timer.callback = callback;
}

void set_clock_rate(double bpm) {
    if (bpm <= 0) bpm = 120.0;

    // clock goes up and down for each beat
    clock_timer.every = timespec_from_double(60 / bpm / 2);
}

struct timespec process_timers() {
    struct timespec next_timer = kNullTimeSpec;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    for (size_t i = 0; i < NUM_TIMERS; i++) {
        event_timer_t *t = timers[i];
        if (timespec_ge(now, t->next)) {
            // fire the callback
            if (t->callback) (*t->callback)();
            t->next = timespec_add(now, t->every);
        }
        if (timespec_gt(t->next, next_timer)) next_timer = t->next;
    }

    return next_timer;
}

void sleep_till_before(struct timespec when) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    if (timespec_le(when, now)) return;

    struct timespec till = timespec_sub(when, now);

    const struct timespec a_little = { .tv_sec = 0, .tv_nsec = 1000 };

    if (timespec_le(till, a_little)) return;

    nanosleep(&till, (struct timespec *)NULL);
}
