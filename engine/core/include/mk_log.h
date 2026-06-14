#ifndef MODERNER_KRIEG_LOG_H
#define MODERNER_KRIEG_LOG_H

#include "mk_core.h"

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MK_LOG_DEBUG = 0,
    MK_LOG_INFO = 1,
    MK_LOG_WARNING = 2,
    MK_LOG_ERROR = 3
} mk_log_level_t;

typedef void (*mk_log_sink_fn)(mk_log_level_t level, const char *message, void *user_data);

const char *mk_result_name(mk_result_t result);
const char *mk_log_level_name(mk_log_level_t level);

void mk_log_set_sink(mk_log_sink_fn sink, void *user_data);
void mk_log_vmessage(mk_log_level_t level, const char *format, va_list args);
void mk_log_message(mk_log_level_t level, const char *format, ...);
void mk_log_result(mk_log_level_t level, const char *action, mk_result_t result);

#ifdef __cplusplus
}
#endif

#endif
