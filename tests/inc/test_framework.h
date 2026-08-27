/**
 * Minimal shared test framework for host unit tests.
 * Same reporting convention as the original suites (PASS/FAIL lines,
 * summary count, non-zero exit code on failure).
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <math.h>
#include <stdio.h>

static int tf_run, tf_pass, tf_fail;

#define CHECK(cond, msg)                                                     \
    do {                                                                     \
        tf_run++;                                                            \
        if (cond) {                                                          \
            tf_pass++;                                                       \
            printf("PASS: %s\n", msg);                                       \
        } else {                                                             \
            tf_fail++;                                                       \
            printf("FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__);           \
        }                                                                    \
    } while (0)

#define CHECK_FLOAT(actual, expected, tol, msg)                              \
    CHECK(fabsf((float)(actual) - (float)(expected)) <= (float)(tol), msg)

static inline int tf_summary(const char *suite)
{
    printf("\n%s: %d run, %d passed, %d failed\n", suite, tf_run, tf_pass, tf_fail);
    return tf_fail ? 1 : 0;
}

#endif /* TEST_FRAMEWORK_H */
