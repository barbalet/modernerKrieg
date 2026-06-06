#include "mk_log.h"

#include <stdio.h>
#include <string.h>

static mk_log_sink_fn mk_log_current_sink = NULL;
static void *mk_log_current_user_data = NULL;

const char *mk_result_name(mk_result_t result) {
    switch (result) {
        case MK_OK:
            return "MK_OK";
        case MK_ERROR_INVALID_ARGUMENT:
            return "MK_ERROR_INVALID_ARGUMENT";
        case MK_ERROR_CAPACITY:
            return "MK_ERROR_CAPACITY";
        case MK_ERROR_NOT_FOUND:
            return "MK_ERROR_NOT_FOUND";
        case MK_ERROR_INVALID_DATA:
            return "MK_ERROR_INVALID_DATA";
        default:
            return "MK_ERROR_UNKNOWN";
    }
}

const char *mk_log_level_name(mk_log_level_t level) {
    switch (level) {
        case MK_LOG_DEBUG:
            return "debug";
        case MK_LOG_INFO:
            return "info";
        case MK_LOG_WARNING:
            return "warning";
        case MK_LOG_ERROR:
            return "error";
        default:
            return "unknown";
    }
}

void mk_log_set_sink(mk_log_sink_fn sink, void *user_data) {
    mk_log_current_sink = sink;
    mk_log_current_user_data = user_data;
}

void mk_log_vmessage(mk_log_level_t level, const char *format, va_list args) {
    char message[1024];
    int written;

    if (format == NULL) {
        return;
    }

    written = vsnprintf(message, sizeof(message), format, args);
    if (written < 0) {
        return;
    }

    message[sizeof(message) - 1] = '\0';

    if (mk_log_current_sink != NULL) {
        mk_log_current_sink(level, message, mk_log_current_user_data);
        return;
    }

    fprintf(stderr, "[%s] %s\n", mk_log_level_name(level), message);
}

void mk_log_message(mk_log_level_t level, const char *format, ...) {
    va_list args;

    va_start(args, format);
    mk_log_vmessage(level, format, args);
    va_end(args);
}

void mk_log_result(mk_log_level_t level, const char *action, mk_result_t result) {
    const char *label = action == NULL ? "operation" : action;

    mk_log_message(level, "%s: %s", label, mk_result_name(result));
}
