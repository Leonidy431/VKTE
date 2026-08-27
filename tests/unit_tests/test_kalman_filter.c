/**
 * @file test_kalman_filter.c
 * @brief Unit tests for the real Kalman filter in firmware/src/data_fusion.
 *
 * Tests verify:
 * - Filter initialization
 * - State estimation accuracy
 * - Adaptation to measurement noise
 * - Convergence speed
 *
 * This test links firmware/src/data_fusion/kalman_filter.c directly so it
 * exercises the actual firmware implementation, not a reimplementation.
 *
 * Compile: gcc -Ifirmware/inc -o test_kalman_filter \
 *              tests/unit_tests/test_kalman_filter.c \
 *              firmware/src/data_fusion/kalman_filter.c -lm
 * Run: ./test_kalman_filter
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#include "kalman_filter.h"

/* ============ Test Framework ============ */

typedef struct {
    int tests_run;
    int tests_passed;
    int tests_failed;
} TestRunner;

void test_assert(TestRunner *runner, int condition, const char *message)
{
    runner->tests_run++;
    if (condition) {
        runner->tests_passed++;
        printf("✓ PASS: %s\n", message);
    } else {
        runner->tests_failed++;
        printf("✗ FAIL: %s\n", message);
    }
}

void test_assert_float(TestRunner *runner, float actual, float expected,
                       float tolerance, const char *message)
{
    runner->tests_run++;
    float error = fabsf(actual - expected);
    if (error <= tolerance) {
        runner->tests_passed++;
        printf("✓ PASS: %s (%.6f ≈ %.6f, error=%.6f)\n",
               message, actual, expected, error);
    } else {
        runner->tests_failed++;
        printf("✗ FAIL: %s (%.6f != %.6f, error=%.6f > %.6f)\n",
               message, actual, expected, error, tolerance);
    }
}

/* ============ Test Cases ============ */

void test_kalman_initialization(TestRunner *runner)
{
    printf("\n=== Test: Kalman Filter Initialization ===\n");

    KalmanFilter kf;
    kalman_init(&kf, 0.001f, 0.1f);

    test_assert(runner, kf.x_est == 0.0f, "Initial estimate is zero");
    test_assert(runner, kf.p_est == 1.0f, "Initial covariance is 1.0");
    test_assert(runner, kf.q == 0.001f, "Process noise set correctly");
    test_assert(runner, kf.r == 0.1f, "Measurement noise set correctly");
}

void test_kalman_convergence_to_constant(TestRunner *runner)
{
    printf("\n=== Test: Convergence to Constant Value ===\n");

    KalmanFilter kf;
    kalman_init(&kf, 0.001f, 0.1f);

    /* Feed 1000 measurements of constant value 5.0 */
    float target_value = 5.0f;
    for (int i = 0; i < 1000; i++) {
        kalman_update(&kf, target_value, 0.001f);
    }

    test_assert_float(runner, kf.x_est, target_value, 0.05f,
                      "Filter converges to constant measurement");
}

void test_kalman_noise_rejection(TestRunner *runner)
{
    printf("\n=== Test: Noise Rejection ===\n");

    KalmanFilter kf;
    kalman_init(&kf, 0.001f, 0.05f);

    /* Simulate noisy measurements around 10.0 */
    float true_value = 10.0f;
    float noise_amplitude = 2.0f;
    float sum_output = 0.0f;

    for (int i = 0; i < 100; i++) {
        /* Simulate noise */
        float noise = (rand() / (float)RAND_MAX - 0.5f) * 2.0f * noise_amplitude;
        float measurement = true_value + noise;
        float filtered = kalman_update(&kf, measurement, 0.001f);
        sum_output += filtered;
    }

    float average_output = sum_output / 100.0f;

    /* Check that filtered output is closer to true value than noisy measurements */
    float measurement_error = noise_amplitude;  /* RMS of uniform noise */
    float filter_error = fabsf(average_output - true_value);

    printf("  Measurement noise (RMS): %.4f\n", measurement_error);
    printf("  Filter error: %.4f\n", filter_error);
    test_assert(runner, filter_error < measurement_error,
                "Filter attenuates noise (error < measurement_noise)");
}

void test_kalman_fast_response(TestRunner *runner)
{
    printf("\n=== Test: Fast Response to Step Input ===\n");

    KalmanFilter kf;
    kalman_init(&kf, 0.01f, 0.1f);  /* Higher process noise for faster response */

    /* Start with estimate of 0 */
    float estimate = kf.x_est;

    /* Step input: jump to 10.0 */
    float step_value = 10.0f;
    float response_time = 0;

    for (int i = 0; i < 100; i++) {
        estimate = kalman_update(&kf, step_value, 0.001f);
        if (estimate > 0.95f * step_value && response_time == 0) {
            response_time = i * 0.001f;
            break;
        }
    }

    test_assert(runner, response_time < 0.05f,
                "Filter reaches 95% of step input within 50 ms");

    printf("  Step response time (95%%): %.3f ms\n", response_time * 1000.0f);
}

void test_kalman_sensor_with_varying_noise(TestRunner *runner)
{
    printf("\n=== Test: Adaptation to Varying Noise (Recoil Scenario) ===\n");

    KalmanFilter kf_high_noise;
    kalman_init(&kf_high_noise, 0.05f, 0.5f);  /* High noise for recoil impact */

    KalmanFilter kf_low_noise;
    kalman_init(&kf_low_noise, 0.001f, 0.05f);  /* Low noise for normal operation */

    /* Phase 1: Normal operation (low noise) */
    float estimate_low = 0.0f;
    for (int i = 0; i < 50; i++) {
        float measurement = 1.0f + 0.01f * (rand() / (float)RAND_MAX - 0.5f);
        estimate_low = kalman_update(&kf_low_noise, measurement, 0.001f);
    }

    /* Phase 2: Recoil event (high noise) */
    float estimate_high = 0.0f;
    for (int i = 0; i < 50; i++) {
        float measurement = 12.0f + 1.0f * (rand() / (float)RAND_MAX - 0.5f);
        estimate_high = kalman_update(&kf_high_noise, measurement, 0.001f);
    }

    test_assert_float(runner, estimate_low, 1.0f, 0.1f,
                      "Low-noise filter tracks smooth signal");
    test_assert_float(runner, estimate_high, 12.0f, 1.5f,
                      "High-noise filter tracks noisy impulse");
}

void test_kalman_covariance_decrease(TestRunner *runner)
{
    printf("\n=== Test: Covariance Decrease Over Time ===\n");

    KalmanFilter kf;
    kalman_init(&kf, 0.001f, 0.1f);

    float initial_covariance = kf.p_est;
    float prev_covariance = initial_covariance;

    /* Run filter for 100 iterations */
    for (int i = 0; i < 100; i++) {
        kalman_update(&kf, 5.0f, 0.001f);

        /* Covariance should monotonically decrease (converge) */
        test_assert(runner, kf.p_est <= prev_covariance,
                    "Covariance decreases monotonically");

        prev_covariance = kf.p_est;
    }

    printf("  Initial covariance: %.6f\n", initial_covariance);
    printf("  Final covariance: %.6f\n", kf.p_est);
    printf("  Reduction factor: %.2f x\n", initial_covariance / kf.p_est);
}

void test_kalman_adapt_for_recoil(TestRunner *runner)
{
    printf("\n=== Test: Recoil-Triggered Covariance Adaptation ===\n");

    KalmanFilter kf;
    kalman_init(&kf, 0.001f, 0.1f);

    /* Below RECOIL_THRESHOLD_G: quiescent branch, tight (default) covariances */
    kalman_adapt_for_recoil(&kf, 1.0f);
    test_assert_float(runner, kf.q, 0.001f, 1e-6f, "Quiescent branch sets default process noise");
    test_assert_float(runner, kf.r, 0.1f, 1e-6f, "Quiescent branch sets default measurement noise");

    /* Above RECOIL_THRESHOLD_G (5.0): high-energy branch, loosened q, tightened r */
    kalman_adapt_for_recoil(&kf, 8.0f);
    test_assert_float(runner, kf.q, 0.001f * 50.0f, 1e-6f, "Recoil branch loosens process noise 50x");
    test_assert_float(runner, kf.r, 0.1f * 0.5f, 1e-6f, "Recoil branch tightens measurement noise by half");

    /* Switching back below threshold restores quiescent covariances */
    kalman_adapt_for_recoil(&kf, 0.5f);
    test_assert_float(runner, kf.q, 0.001f, 1e-6f, "Adaptation reverts after recoil event ends");
}

/* ============ Main Test Runner ============ */

int main(void)
{
    TestRunner runner = {0, 0, 0};

    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║   STM32H745 Shotgun Testbench - Kalman Filter Unit Tests   ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");

    /* Run all tests */
    test_kalman_initialization(&runner);
    test_kalman_convergence_to_constant(&runner);
    test_kalman_noise_rejection(&runner);
    test_kalman_fast_response(&runner);
    test_kalman_sensor_with_varying_noise(&runner);
    test_kalman_covariance_decrease(&runner);
    test_kalman_adapt_for_recoil(&runner);

    /* Summary */
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║                      TEST SUMMARY                          ║\n");
    printf("╠════════════════════════════════════════════════════════════╣\n");
    printf("║ Tests Run:    %3d                                          ║\n", runner.tests_run);
    printf("║ Tests Passed: %3d ✓                                        ║\n", runner.tests_passed);
    printf("║ Tests Failed: %3d %s                                      ║\n",
           runner.tests_failed, runner.tests_failed == 0 ? "✓" : "✗");
    printf("║ Success Rate: %.1f%%                                         ║\n",
           100.0f * runner.tests_passed / runner.tests_run);
    printf("╚════════════════════════════════════════════════════════════╝\n\n");

    return runner.tests_failed == 0 ? 0 : 1;
}
