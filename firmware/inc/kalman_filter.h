/**
 * @file kalman_filter.h
 * @brief Public interface for the adaptive accelerometer Kalman filter.
 */

#ifndef __KALMAN_FILTER_H__
#define __KALMAN_FILTER_H__

#include "types.h"

/**
 * Initialize a filter instance.
 * @param kf                 Filter state to initialize.
 * @param process_noise_q    Process noise covariance (model uncertainty).
 * @param measurement_noise_r Measurement noise covariance (sensor uncertainty).
 */
void kalman_init(KalmanFilter *kf, float process_noise_q, float measurement_noise_r);

/**
 * Advance the filter by one measurement.
 * @param kf  Filter state.
 * @param z   New measurement.
 * @param dt  Time step in seconds (reserved).
 * @return    Filtered estimate.
 */
float kalman_update(KalmanFilter *kf, float z, float dt);

/**
 * Adapt covariances based on the current recoil amplitude.
 * @param kf                Filter state.
 * @param peak_amplitude_g  Current acceleration magnitude in g.
 */
void kalman_adapt_for_recoil(KalmanFilter *kf, float peak_amplitude_g);

#endif /* __KALMAN_FILTER_H__ */
