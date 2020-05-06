#include "csound.h"

#include <stdbool.h>

#include <csound/csound.h>

#include "orchestras.h"

static bool quit_csound_now = false;
static void *cs_thread_id;

// Csound thread
typedef struct {
    CSOUND *csound;
} cs_user_data_t;
cs_user_data_t cs_user_data = {};

static uintptr_t cs_thread(void *data) {
    cs_user_data_t *ud = (cs_user_data_t *)data;
    while ((csoundPerformKsmps(ud->csound) == 0) &&
           (quit_csound_now == false)) {
    }
    csoundDestroy(ud->csound);
    return 1;
}

static void no_message_callback(__attribute__((unused)) CSOUND *cs,
                                __attribute__((unused)) int attr,
                                __attribute__((unused)) const char *format,
                                __attribute__((unused)) va_list valist) {
    // Do nothing so that Csound will not print any message,
    // leaving a clean console for our app
    return;
}

void start_csound() {
    // Csound set up
    // silence Csound messages
    csoundSetDefaultMessageCallback(no_message_callback);
    csoundInitialize(CSOUNDINIT_NO_ATEXIT | CSOUNDINIT_NO_SIGNAL_HANDLER);
    cs_user_data.csound = csoundCreate(NULL);

    csoundSetOption(cs_user_data.csound, "-odac");
    csoundSetOption(cs_user_data.csound, "-d");
    if (csoundCompileOrc(cs_user_data.csound, simple_trigger_orc) != 0) {
        printf("Orchestra compile failed\n");
        exit(-1);
    }
    csoundStart(cs_user_data.csound);
    cs_thread_id = csoundCreateThread(cs_thread, &cs_user_data);
}

void stop_csound() {
    quit_csound_now = true;
    csoundJoinThread(cs_thread_id);
    csoundDestroy(cs_user_data.csound);
}

void csound_set_trigger_output(uint8_t idx, bool state) {
    const char *i[] = { "1", "2", "3", "4", "5", "6", "7", "8" };
    const char *const n[] = { "9.00", "9.02", "9.04", "9.05",
                              "9.07", "9.09", "9.11", "10.00" };
    if (state) {
        // "i 1.1 0 -1 9.04"
        char m[255] = "i 1.";
        strcat(m, i[idx]);
        strcat(m, " 0 -1 ");
        strcat(m, n[idx]);
        csoundInputMessageAsync(cs_user_data.csound, m);
    }
    else {
        // "i -1.1 0 0"
        char m[255] = "i -1.";
        strcat(m, i[idx]);
        strcat(m, " 0 0");
        csoundInputMessageAsync(cs_user_data.csound, m);
    }
}
