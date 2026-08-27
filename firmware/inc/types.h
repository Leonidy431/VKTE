/**
 * @file types.h
 * @brief Custom data types and structures for sensor data and commands.
 */

#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdint.h>
#include <stddef.h>

/* ============ IMU Data Structure ============ */
typedef struct {
    float accel_x_g;      /* Acceleration X-axis (g) */
    float accel_y_g;      /* Acceleration Y-axis (g) */
    float accel_z_g;      /* Acceleration Z-axis (g) */
    float gyro_x_dps;     /* Angular velocity X-axis (°/s) */
    float gyro_y_dps;     /* Angular velocity Y-axis (°/s) */
    float gyro_z_dps;     /* Angular velocity Z-axis (°/s) */
    uint32_t timestamp_us; /* Microsecond timestamp */
} IMUData;

/* ============ Sensor Data Packet (128 bytes) ============ */
typedef struct {
    uint32_t sequence_number;   /* Packet sequence for loss detection */
    uint32_t timestamp_ms;      /* Millisecond timestamp */
    uint32_t timestamp_us_fine; /* Microsecond refinement */

    /* IMU data */
    float accel_x_g;
    float accel_y_g;
    float accel_z_g;
    float gyro_x_dps;
    float gyro_y_dps;
    float gyro_z_dps;

    /* Environmental sensors */
    float distance_m;       /* LRF distance measurement */
    float barrel_temp_c;    /* Thermistor reading */
    float env_temp_c;       /* BME280 ambient temperature */
    float pressure_hpa;     /* Barometric pressure */

    /* Status and diagnostics */
    uint32_t status_flags;  /* System status bits */
    uint32_t reserved1;
    uint32_t reserved2;

    /* Integrity check */
    uint32_t crc32;         /* CRC32 for data integrity */
} SensorDataPacket;

/* ============ Shot Event Structure ============ */
typedef struct {
    uint32_t shot_id;           /* Sequential shot number */
    uint32_t timestamp_ms;      /* Time of shot event */
    uint32_t timestamp_us;      /* Fine microsecond timestamp */

    /* Ballistic parameters */
    float distance_m;
    float barrel_temp_c;
    float env_temp_c;
    float pressure_hpa;

    /* Recoil analysis */
    float recoil_peak_g;        /* Peak acceleration magnitude */
    float recoil_duration_ms;   /* Recoil impulse duration */
    float recoil_direction_deg; /* Direction of recoil vector */

    /* Target impact */
    int16_t hit_x_mm;           /* Impact X coordinate (mm) */
    int16_t hit_y_mm;           /* Impact Y coordinate (mm) */
    int16_t hit_distance_mm;    /* Distance from center (mm) */

    /* Ammo and session data */
    uint8_t ammo_type_id;       /* Ammunition profile ID */
    uint8_t reserved;
    uint16_t session_id;        /* Current session identifier */
} ShotEvent;

/* ============ Command Packet (64 bytes) ============ */
typedef struct {
    uint8_t command_id;         /* Command type */
    uint8_t flags;              /* Command flags/options */
    uint16_t reserved;

    /* Command parameters */
    uint32_t param1;
    uint32_t param2;
    uint32_t param3;

    /* Payload */
    uint8_t data_payload[48];   /* Command-specific data */

    /* Integrity */
    uint8_t crc8;               /* Simple CRC8 */
} CommandPacket;

/* ============ Command IDs ============ */
typedef enum {
    CMD_NOOP = 0x00,
    CMD_START_SESSION = 0x01,
    CMD_STOP_SESSION = 0x02,
    CMD_CALIBRATE_IMU = 0x03,
    CMD_CALIBRATE_THERMISTOR = 0x04,
    CMD_SET_THERMAL_THRESHOLD = 0x05,
    CMD_SAVE_PROFILE = 0x06,
    CMD_LOAD_PROFILE = 0x07,
    CMD_QUERY_STATUS = 0x08,
    CMD_DOWNLOAD_LOGS = 0x09,
} CommandID;

/* ============ System State Enum ============ */
typedef enum {
    STATE_BOOT = 0,
    STATE_IDLE = 1,
    STATE_SESSION_ACTIVE = 2,
    STATE_CALIBRATING = 3,
    STATE_ERROR = 4,
    STATE_SLEEP = 5,
} SystemState;

/* ============ Kalman Filter State ============ */
typedef struct {
    float x_est;           /* Estimated value */
    float p_est;           /* Estimate error covariance */
    float q;               /* Process noise covariance */
    float r;               /* Measurement noise covariance */
} KalmanFilter;

/* ============ Session Configuration ============ */
typedef struct {
    uint16_t session_id;
    uint32_t timestamp_start;
    char session_name[32];
    uint8_t ammo_type_id;
    float expected_distance_m;
    uint16_t expected_shots;
} SessionConfig;

/* ============ Thermal Compensation Data ============ */
typedef struct {
    float baseline_temp_c;      /* Reference temperature */
    float poi_shift_per_degree; /* POI shift (mm/°C) */
    float grouping_degradation; /* Grouping spread increase rate */
} ThermalModel;

/* ============ Status Flags ============ */
typedef enum {
    STATUS_IMU_READY = (1 << 0),
    STATUS_LRF_READY = (1 << 1),
    STATUS_FLASH_READY = (1 << 2),
    STATUS_UART_READY = (1 << 3),
    STATUS_CALIBRATED = (1 << 4),
    STATUS_LOW_BATTERY = (1 << 5),
    STATUS_THERMAL_WARNING = (1 << 6),
    STATUS_ERROR = (1 << 7),
} StatusFlags;

#endif /* __TYPES_H__ */
