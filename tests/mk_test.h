#ifndef MODERNER_KRIEG_TEST_H
#define MODERNER_KRIEG_TEST_H

#include "mk_core.h"

#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char text[4096];
    size_t length;
} mk_test_transcript_t;

static inline void mk_test_fail_at(const char *file, int line, const char *expression) {
    fprintf(stderr, "%s:%d: assertion failed: %s\n", file, line, expression);
    abort();
}

#define MK_TEST_ASSERT(expression) \
    do { \
        if (!(expression)) { \
            mk_test_fail_at(__FILE__, __LINE__, #expression); \
        } \
    } while (0)

static inline void mk_test_assert_close_at(
    const char *file,
    int line,
    float actual,
    float expected,
    float tolerance
) {
    if (fabsf(actual - expected) > tolerance) {
        fprintf(
            stderr,
            "%s:%d: expected %.4f within %.4f, got %.4f\n",
            file,
            line,
            expected,
            tolerance,
            actual
        );
        abort();
    }
}

#define MK_TEST_ASSERT_CLOSE(actual, expected) \
    mk_test_assert_close_at(__FILE__, __LINE__, (actual), (expected), 0.01f)

static inline void mk_test_transcript_init(mk_test_transcript_t *transcript) {
    if (transcript == NULL) {
        return;
    }

    transcript->text[0] = '\0';
    transcript->length = 0;
}

static inline void mk_test_transcript_append(mk_test_transcript_t *transcript, const char *format, ...) {
    va_list args;
    int written;
    size_t remaining;

    if (transcript == NULL || format == NULL || transcript->length >= sizeof(transcript->text)) {
        return;
    }

    remaining = sizeof(transcript->text) - transcript->length;
    va_start(args, format);
    written = vsnprintf(transcript->text + transcript->length, remaining, format, args);
    va_end(args);

    if (written < 0) {
        return;
    }

    if ((size_t)written >= remaining) {
        transcript->length = sizeof(transcript->text) - 1;
        transcript->text[transcript->length] = '\0';
        return;
    }

    transcript->length += (size_t)written;
}

static inline bool mk_test_transcript_contains(const mk_test_transcript_t *transcript, const char *needle) {
    if (transcript == NULL || needle == NULL) {
        return false;
    }

    return strstr(transcript->text, needle) != NULL;
}

static inline mk_vec2_t mk_test_vec2(float x, float y) {
    mk_vec2_t value;

    value.x = x;
    value.y = y;

    return value;
}

#endif
