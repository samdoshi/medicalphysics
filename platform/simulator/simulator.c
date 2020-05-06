#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <monome.h>

#include "app.h"
#include "csound.h"
#include "hardware.h"
#include "state.h"
#include "timers.h"

// flag to indicate that the simulator should quit (to help Csound quit)
static bool quit_now = false;

// hardware has no concept of playing notes like Csound does,
// this is used to compensate for that
static bool trigger_playing[8] = { false };

// grid status
#define GRID_SIZE 16
static uint8_t grid[GRID_SIZE][GRID_SIZE] = { { 0 } };

#define QUADRANTS 4
static bool quadrant_dirty[QUADRANTS] = { false };

static monome_t *monome;
static state_t state;

void hardware_set_clock_output(bool val) {
    if (!val) printf("\n");
}

void hardware_set_trigger_output(uint8_t idx, bool val) {
    if (val) {
        printf("T%d", idx);
        trigger_playing[idx] = true;
        csound_set_trigger_output(idx, true);
    }
    else if (trigger_playing[idx] == true) {
        trigger_playing[idx] = false;
        csound_set_trigger_output(idx, false);
    }
}

void grid_set_dirty(uint8_t quadrant) {
    if (quadrant >= QUADRANTS) return;
    quadrant_dirty[quadrant] = true;
}

void grid_arc_clear(void) {
    memset(grid, 0, GRID_SIZE * GRID_SIZE);
}

void grid_set(uint8_t x, uint8_t y, uint8_t level) {
    if (x >= GRID_SIZE || y >= GRID_SIZE) return;
    grid[y][x] = level;
}

void grid_refresh() {
    for (uint8_t q = 0; q < 4; q++) {
        if (quadrant_dirty[q] == false) continue;

        uint8_t off_x = 0;
        uint8_t off_y = 0;
        switch (q) {
            case 0:
                off_x = 0;
                off_y = 0;
                break;
            case 1:
                off_x = 8;
                off_y = 0;
                break;
            case 2:
                off_x = 8;
                off_y = 0;
                break;
            case 3:
                off_x = 8;
                off_y = 8;
                break;
        }
        uint8_t data[64];
        for (uint8_t y = 0; y < 8; y++) {
            for (uint8_t x = 0; x < 8; x++) {
                data[y * 8 + x] = grid[y + off_y][x + off_x];
            }
        }
        monome_led_level_map(monome, off_x, off_y, data);
        quadrant_dirty[q] = false;
    }
}

static void handle_press(const monome_event_t *e, void *user_data) {
    uint8_t x = (uint8_t)e->grid.x;
    uint8_t y = (uint8_t)e->grid.y;
    uint8_t z = (uint8_t)user_data;
    app_grid_press(&state, x, y, z);
}

static void handle_clock() {
    static bool clock = false;  // start clock low
    app_clock(&state, clock);
    clock = !clock;
}

static void handle_refresh() {
    if (state_is_ui_dirty(&state)) app_refresh(&state);
}

static void set_quit_now() {
    quit_now = true;
}

int main() {
    monome = monome_open("/dev/ttyUSB0");
    if (!monome) {
        printf("Connection failed\n");
        return -1;
    }

    // Ctrl-C handler
    signal(SIGINT, set_quit_now);

    start_csound();

    monome_register_handler(monome, MONOME_BUTTON_DOWN, handle_press,
                            (void *)0);
    monome_register_handler(monome, MONOME_BUTTON_UP, handle_press, (void *)1);

    setbuf(stdout, NULL);

    set_clock_callback(handle_clock);
    set_clock_rate(120.0 * 8);
    set_refresh_callback(handle_refresh);

    while (!quit_now) {
        while (monome_event_handle_next(monome)) {
        }
        struct timespec next = process_timers();
        sleep_till_before(next);
    }

    monome_led_all(monome, 0);
    monome_close(monome);

    stop_csound();

    return 0;
};
