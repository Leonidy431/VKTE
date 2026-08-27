# STM32H745 Ballistic Corrector — Implementation Backlog & Technical Specifications

**Date:** 2026-07-28  
**Status:** 🚀 ACTIVE IMPLEMENTATION  
**Priority:** Critical path items first

---

## 📋 Backlog Overview

Total items: **47 implementations**  
Status breakdown:
- ✅ Complete: 8 (firmware core + dual-core)
- 🔄 In Progress: 0
- ⏳ Ready to implement: 39

---

## Phase 1: Firmware Core (M7 Real-Time Sensor Fusion)

### ✅ COMPLETE: M7 Architecture & Initialization

**File:** `firmware/src/main.c`  
**Status:** ✅ COMPLETE

```c
// Main entry point - dual-core initialization
// - Clock configuration (480 MHz M7, 240 MHz M4)
// - RTOS scheduler (FreeRTOS)
// - Dual-core communication setup
// - Watchdog initialization (IWDG + WWDG)
// - Power management setup
```

**Done:**
- [x] M7 clock tree (PLL 480 MHz)
- [x] M4 clock configuration (240 MHz)
- [x] Shared memory initialization (AXI-SRAM 0x2007C000)
- [x] Dual-core handshake protocol
- [x] UART debug initialization (921.6k)

---

### ✅ COMPLETE: Kalman Filter Implementation

**File:** `firmware/src/data_fusion/kalman_filter.c`  
**Status:** ✅ COMPLETE

**Specifications:**
```
Algorithm: Extended Kalman Filter (EKF)
Sampling rate: 1 kHz (accelerometer)
State vector: [ax, ay, az] - 3D acceleration
Measurement: 9-axis IMU (accel + gyro + mag)
Adaptive covariances: Yes (per-axis Q/R tuning)
Performance: <20ms latency, <0.5g RMS error
```

**Done:**
- [x] EKF state prediction
- [x] Measurement update (accelerometer)
- [x] Covariance matrix updates
- [x] Gyro integration for angular velocity
- [x] Outlier rejection (3-sigma gates)
- [x] Adaptive tuning based on residuals

---

### ✅ COMPLETE: Shot Detection State Machine

**File:** `firmware/src/shot_detection/shot_fsm.c`  
**Status:** ✅ COMPLETE

**Specifications:**
```
States: IDLE → ARMED → COLLECTING → COOLDOWN → IDLE
Trigger: Acceleration >2g threshold (Kalman-filtered)
Collection: 500ms window (post-trigger sampling)
Hysteresis: 0.1g to prevent false triggers
Output: Shot event with timestamp + accel data
```

**Done:**
- [x] State machine transitions
- [x] Threshold detection (configurable)
- [x] Data buffering (500ms @ 1kHz = 500 samples)
- [x] Timestamp annotation (µs precision)
- [x] Hysteresis implementation

---

### ✅ COMPLETE: Anomaly Detection (Over-Pressure/Thermal)

**File:** `firmware/src/data_fusion/anomaly_detector.c`  
**Status:** ✅ COMPLETE

**Specifications:**
```
Method: EWMA (Exponential Weighted Moving Average)
Channels: Recoil (acceleration), barrel temperature
Thresholds:
  - Warning: 2.5× baseline mean
  - Critical: 18g absolute (over-pressure)
  - Thermal: 1.3× mean temperature + 5°C
Alpha (smoothing): 0.1 (recoil), 0.05 (temperature)
Update rate: Per-shot (event-driven)
```

**Done:**
- [x] EWMA state initialization
- [x] Per-shot update logic
- [x] Multi-threshold detection
- [x] Event logging (anomaly type + value)
- [x] Recovery hysteresis (10-shot cooldown)

---

### ✅ COMPLETE: Thermal Drift Compensation

**File:** `firmware/src/data_fusion/thermal_robust.c`  
**Status:** ✅ COMPLETE

**Specifications:**
```
Algorithm: Huber robust regression (outlier-resistant)
Loss function: Huber loss with k=2.0×sigma
Iteration: 3-5 rounds until convergence
Outlier detection: 3-sigma Median Absolute Deviation
Prediction: POI shift (mm) vs. barrel temperature (°C)
Accuracy target: ±0.1mm over -10 to +100°C range
```

**Done:**
- [x] Huber loss function
- [x] Iterative reweighting
- [x] MAD-based sigma estimation
- [x] Outlier detection & masking
- [x] Temperature-POI polynomial fitting (2nd order)
- [x] Shift prediction function

---

## Phase 2: Hardware Integration & Sensor Drivers

### ⏳ IMPLEMENT: Sensor Interface Drivers

**Priority:** HIGH  
**Estimated effort:** 4 hours

#### 2.1: ICM-20689 IMU Driver (SPI @ 10 MHz)

**File:** `firmware/src/drivers/icm20689.c`  
**Status:** ⏳ IMPLEMENT

**Technical Specification (TZ):**
```c
// ICM-20689 Register Map & Configuration
#define ICM20689_WHO_AM_I          0x75  // Expected: 0x68
#define ICM20689_ACCEL_XOUT_H      0x3B  // Start of 6-byte accel data
#define ICM20689_GYRO_XOUT_H       0x43  // Start of 6-byte gyro data
#define ICM20689_TEMP_OUT_H        0x41  // 2-byte temperature

// Configuration registers
#define ICM20689_PWR_MGMT_1        0x6B  // Clock source, sleep mode
#define ICM20689_PWR_MGMT_2        0x6C  // Sensor enable/disable
#define ICM20689_CONFIG            0x1A  // DLPF configuration
#define ICM20689_ACCEL_CONFIG      0x1C  // Accel range ±16g, ±8g, ±4g, ±2g
#define ICM20689_ACCEL_CONFIG_2    0x1D  // Accel DLPF
#define ICM20689_GYRO_CONFIG       0x1B  // Gyro range ±2000°/s

// Sampling rate divider: f_sample = f_internal / (1 + SMPLRT_DIV)
#define ICM20689_SMPLRT_DIV        0x19  // Target: 1kHz (divider = 9 for 10kHz internal)

// SPI transfer format
#define ICM20689_SPI_READ          0x80  // Read bit (MSB set)
#define ICM20689_SPI_WRITE         0x00  // Write bit (MSB clear)

// Data structure for raw sensor readout
typedef struct {
    int16_t accel_x, accel_y, accel_z;  // ±16g raw (LSB = ±2000/32768 m/s²)
    int16_t gyro_x, gyro_y, gyro_z;     // ±2000°/s raw (LSB = 2000/32768 °/s)
    int16_t temp_raw;                    // Temperature raw (LSB = 1/340°C + 36.53°C offset)
    uint64_t timestamp_us;               // Microsecond timestamp
} icm20689_data_t;

// Function prototypes
int icm20689_init(void);                 // Initialize SPI + configure registers
int icm20689_read_who_am_i(uint8_t *id); // Verify device ID (expect 0x68)
int icm20689_read_data(icm20689_data_t *data); // Blocking read (6 accel + 2 temp + 6 gyro = 14 bytes)
int icm20689_set_accel_range(uint8_t range); // 0=±2g, 1=±4g, 2=±8g, 3=±16g
int icm20689_set_gyro_range(uint8_t range);  // 0=±250°/s, 1=±500°/s, 2=±1000°/s, 3=±2000°/s
int icm20689_self_test(void);            // Return 0 if pass, -1 if fail
float icm20689_accel_to_g(int16_t raw_accel); // Convert raw to Gs (for ±16g range)
```

**Implementation checklist:**
- [ ] SPI initialization (PA5/PA6/PA7 + PA4 CS)
- [ ] Device ID verification (WHO_AM_I register)
- [ ] Clock source configuration (PLL internal oscillator)
- [ ] Accelerometer range setup (±16g, 16-bit)
- [ ] Gyro range setup (±2000°/s)
- [ ] DLPF configuration (20Hz cutoff for noise rejection)
- [ ] Sampling rate divider (1kHz output)
- [ ] Data read function (blocking SPI transaction)
- [ ] Self-test procedure
- [ ] Error handling (SPI timeout, invalid data)

---

#### 2.2: VL53L0X Rangefinder Driver (I2C @ 400 kHz)

**File:** `firmware/src/drivers/vl53l0x.c`  
**Status:** ⏳ IMPLEMENT

**Technical Specification (TZ):**
```c
// VL53L0X I2C Address & Registers
#define VL53L0X_I2C_ADDR           0x29  // Fixed I2C address

// Register map (subset for basic ranging)
#define VL53L0X_IDENTIFICATION_MODEL_ID  0xC0  // Expected: 0xEE
#define VL53L0X_RESULT_RANGE_STATUS      0x14  // Range data ready flag
#define VL53L0X_RESULT_CORE_RANGING_TOTAL_TRIES_TO_TIMEOUT 0x24
#define VL53L0X_RESULT_CORE_AMBIENT_WINDOW_EVENTS_RTN 0x23
#define VL53L0X_RESULT_CORE_RANGING_TOTAL_EVENTS_RTN  0x21
#define VL53L0X_RESULT_CORE_SIGNAL_RATE_REF_MCPS 0x20
#define VL53L0X_RESULT_RANGE_MILLIMETERS 0x1F // 2-byte range in mm

// Measurement timing
#define VL53L0X_MEASUREMENT_TIMING_BUDGET 33000 // 33ms measurement window
#define VL53L0X_PRE_RANGE_CONFIG_TIMEOUT  0xFF00 // Timeout in VCSEL periods

// Data structure for range measurement
typedef struct {
    uint16_t range_mm;                  // Distance in millimeters (30-1200mm valid)
    uint16_t signal_rate;               // Signal rate in MCPS (×65536)
    uint8_t range_status;               // Status code (0=valid, other=error)
    uint64_t timestamp_us;              // Measurement timestamp
} vl53l0x_range_t;

// Function prototypes
int vl53l0x_init(void);                 // Initialize I2C + configure
int vl53l0x_read_model_id(uint8_t *id); // Verify device (expect 0xEE)
int vl53l0x_start_continuous(void);     // Start continuous measurement
int vl53l0x_stop_continuous(void);      // Stop continuous measurement
int vl53l0x_read_range(vl53l0x_range_t *range); // Blocking read of latest range
int vl53l0x_is_data_ready(void);        // Non-blocking data ready check
int vl53l0x_set_measurement_timing(uint32_t timing_us); // Set measurement window
```

**Implementation checklist:**
- [ ] I2C initialization (PB10/PB11 @ 400 kHz)
- [ ] Device ID verification (0xEE)
- [ ] GPIO0/XSHUT pin setup (active high enable)
- [ ] Mode selection (continuous ranging, long distance)
- [ ] Measurement timing configuration (33ms window)
- [ ] Timeout configuration (prevent VCSEL lockout)
- [ ] Data ready polling loop
- [ ] Range register read (14h-1Fh, 14 bytes)
- [ ] Range status parsing (0=valid, 1=sigma fail, 4=phase fail, 24=min range fail)
- [ ] Error handling (I2C timeout, out-of-range values)

---

#### 2.3: MCP9808 Temperature Sensor Driver (I2C @ 400 kHz)

**File:** `firmware/src/drivers/mcp9808.c`  
**Status:** ⏳ IMPLEMENT

**Technical Specification (TZ):**
```c
// MCP9808 I2C Address (with A0/A1/A2 address pins)
#define MCP9808_I2C_ADDR_BASE      0x60  // Base address (A0/A1/A2 = 0)
#define MCP9808_I2C_ADDR_ACTUAL    0x60  // Our configuration: all address pins GND

// Register map
#define MCP9808_CONFIG             0x01  // Config register (hysteresis, shutdown, etc.)
#define MCP9808_UPPER_TEMP         0x02  // Upper alert temperature threshold
#define MCP9808_LOWER_TEMP         0x03  // Lower alert temperature threshold
#define MCP9808_CRIT_TEMP          0x04  // Critical temperature threshold
#define MCP9808_TA                 0x05  // Ambient temperature register (read-only)
#define MCP9808_MANUFACTURER_ID    0x06  // Expected: 0x0054
#define MCP9808_DEVICE_ID          0x07  // Expected: 0x0400
#define MCP9808_RESOLUTION         0x08  // Resolution: 0=0.5°C, 1=0.25°C, 2=0.125°C, 3=0.0625°C

// Temperature register format (13-bit, 2's complement)
// Bits 15-12: Sign + upper bits (integer part)
// Bits 11-8: Lower nibble of integer
// Bits 7-4: Fractional part (0.0625°C per bit)
// Bits 3-0: Reserved (0)

// Data structure for temperature reading
typedef struct {
    float temp_celsius;                 // Ambient temperature in °C (±0.0625°C resolution)
    uint8_t upper_alert;                // Alert flags: upper/lower/crit bit status
    uint64_t timestamp_us;              // Measurement timestamp
} mcp9808_temp_t;

// Function prototypes
int mcp9808_init(void);                 // Initialize I2C + configure (resolution 0.0625°C)
int mcp9808_read_id(uint16_t *id);      // Verify device (expect 0x0400)
int mcp9808_read_temperature(mcp9808_temp_t *temp); // Blocking read
int mcp9808_set_resolution(uint8_t res); // 0=0.5°C, 1=0.25°C, 2=0.125°C, 3=0.0625°C
int mcp9808_set_alert_thresholds(float upper, float lower, float critical);
float mcp9808_raw_to_celsius(uint16_t raw);  // Convert 13-bit raw to float
```

**Implementation checklist:**
- [ ] I2C initialization (PB10/PB11)
- [ ] Device ID verification (0x0400)
- [ ] Resolution setup (0.0625°C, 3 conversion cycles)
- [ ] Hysteresis configuration (±1.5°C default)
- [ ] Alert threshold setup (upper=60°C, lower=50°C, critical=75°C)
- [ ] Temperature register read (2 bytes, 13-bit format)
- [ ] Raw data conversion (13-bit → float Celsius)
- [ ] Alert flag parsing
- [ ] Continuous vs. one-shot mode (we use continuous)
- [ ] Error handling (I2C timeout, invalid data)

---

#### 2.4: BMP390 Pressure/Temperature Sensor Driver (I2C/SPI)

**File:** `firmware/src/drivers/bmp390.c`  
**Status:** ⏳ IMPLEMENT

**Technical Specification (TZ):**
```c
// BMP390 I2C/SPI Address
#define BMP390_I2C_ADDR            0x77  // Primary address (SDO pin = GND)

// Register map (subset for pressure + temperature)
#define BMP390_CHIP_ID             0x00  // Expected: 0x60
#define BMP390_REV_ID              0x01  // Expected: 0x01
#define BMP390_ERR_REG             0x02  // Error flags
#define BMP390_STATUS              0x03  // Status: measuring, measuring NVM, NVM ready
#define BMP390_PRESS_XLSB          0x04  // Pressure XLSB
#define BMP390_PRESS_LSB           0x05  // Pressure LSB
#define BMP390_PRESS_MSB           0x06  // Pressure MSB (3 bytes total, 20-bit)
#define BMP390_TEMP_XLSB           0x07  // Temperature XLSB
#define BMP390_TEMP_LSB            0x08  // Temperature LSB
#define BMP390_TEMP_MSB            0x09  // Temperature MSB (3 bytes total, 20-bit)
#define BMP390_SENSORTIME          0x0C  // Sensor time (3 bytes, 24-bit, 1µs/LSB)
#define BMP390_EVENT               0x10  // Event (FIFO full, data ready, config error)
#define BMP390_INT_STATUS          0x11  // Interrupt status
#define BMP390_FIFO_LENGTH         0x12  // FIFO length (11 bits)
#define BMP390_FIFO_DATA           0x14  // FIFO data read
#define BMP390_FIFO_CONFIG_1       0x17  // FIFO watermark level
#define BMP390_FIFO_CONFIG_2       0x18  // FIFO mode
#define BMP390_INT_CTRL            0x19  // Interrupt output drive + polarity
#define BMP390_IF_CONF             0x1A  // SPI 4-wire vs. 3-wire, I2C watchdog
#define BMP390_PWR_CTRL            0x1B  // Power mode (sleep/normal/forced)
#define BMP390_OSR                 0x1C  // Over-sampling ratio (pressure + temperature)
#define BMP390_ODR                 0x1D  // Output data rate
#define BMP390_CONFIG              0x1F  // IIR filter coefficient
#define BMP390_CALIB_DATA          0x31  // Start of calibration data (21 bytes)
#define BMP390_CMD                 0x7E  // Command register (soft reset = 0xB6)

// Data structure for pressure/temperature reading
typedef struct {
    float pressure_pa;                  // Absolute pressure in Pa (target: 20-110 kPa ambient)
    float temperature_celsius;          // Temperature in °C (±1°C accuracy)
    uint32_t sensor_time_us;            // Sensor internal time in microseconds
    uint8_t status;                     // Status flags
    uint64_t timestamp_us;              // Host timestamp
} bmp390_data_t;

// Calibration data structure (loaded from NVM)
typedef struct {
    int16_t t1, t2, t3;                 // Temperature calibration coefficients
    int16_t p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11; // Pressure calibration
} bmp390_calib_t;

// Function prototypes
int bmp390_init(void);                  // Initialize I2C + load calibration
int bmp390_read_id(uint8_t *id);        // Verify device (expect 0x60)
int bmp390_soft_reset(void);            // Send soft reset command (0xB6)
int bmp390_read_data(bmp390_data_t *data); // Blocking read pressure + temperature
int bmp390_set_oversample(uint8_t osr); // 0=skip, 1=1x, 2=2x, 3=4x, 4=8x, 5=16x, 6=32x
int bmp390_set_odr(uint8_t odr);        // Output data rate: 1.5-200 Hz
int bmp390_read_calib_data(bmp390_calib_t *calib); // Load calibration from NVM
float bmp390_compensate_temperature(uint32_t temp_raw, bmp390_calib_t *calib); // Calibrated temp
float bmp390_compensate_pressure(uint32_t pres_raw, bmp390_calib_t *calib, float t_comp); // Calibrated pressure
int bmp390_read_status(uint8_t *status); // Check measuring/NVM ready flags
```

**Implementation checklist:**
- [ ] I2C initialization (PB10/PB11)
- [ ] Device ID verification (0x60)
- [ ] Soft reset (0xB6 to command register)
- [ ] Calibration data load (21 bytes from 0x31-0x45)
- [ ] Over-sampling configuration (pressure 16×, temperature 2×)
- [ ] Output data rate setup (50 Hz)
- [ ] IIR filter coefficient (0x04 = 0.5 filter)
- [ ] Power mode (normal: 0x03)
- [ ] Measurement status polling
- [ ] Raw data read (6 bytes: 3 pressure + 3 temperature)
- [ ] Temperature compensation (floating-point)
- [ ] Pressure compensation (floating-point with calibration)
- [ ] Error handling (I2C, invalid calibration data)

---

### ⏳ IMPLEMENT: Flash Memory & EEPROM Drivers

**Priority:** HIGH  
**Estimated effort:** 6 hours

#### 2.5: W25Q128JV QSPI Flash Driver

**File:** `firmware/src/drivers/flash_qspi.c`  
**Status:** ⏳ IMPLEMENT

**Technical Specification (TZ):**
```c
// W25Q128JV QSPI Flash (16 MB, 104 MHz quad I/O)
#define W25Q128_JEDEC_ID            0xEF4018  // Expected JEDEC ID
#define W25Q128_TOTAL_SIZE          (16 * 1024 * 1024) // 16 MB
#define W25Q128_SECTOR_SIZE         (4 * 1024)  // 4 KB erase sector
#define W25Q128_PAGE_SIZE           256         // 256-byte write page
#define W25Q128_MAX_SECTORS         (W25Q128_TOTAL_SIZE / W25Q128_SECTOR_SIZE) // 4096 sectors

// QSPI Commands (single SPI mode first, quad mode later)
#define W25Q_CMD_RESET              0xFF  // Reset enable
#define W25Q_CMD_READ_ID            0x9F  // Read JEDEC ID (3 bytes)
#define W25Q_CMD_READ               0x03  // Read data @ SPI single-I/O (25 MHz max)
#define W25Q_CMD_FAST_READ          0x0B  // Fast read @ 50 MHz (dummy byte)
#define W25Q_CMD_FAST_READ_DUAL     0x3B  // Dual I/O read @ 80 MHz
#define W25Q_CMD_FAST_READ_QUAD     0x6B  // Quad I/O read @ 104 MHz
#define W25Q_CMD_PAGE_PROGRAM       0x02  // Write page (256 bytes max)
#define W25Q_CMD_QUAD_PAGE_PROGRAM  0x32  // Quad I/O page write @ 104 MHz
#define W25Q_CMD_SECTOR_ERASE       0x20  // Erase 4KB sector (100ms typical)
#define W25Q_CMD_BLOCK32_ERASE      0x52  // Erase 32KB block (800ms typical)
#define W25Q_CMD_BLOCK64_ERASE      0xD8  // Erase 64KB block (1s typical)
#define W25Q_CMD_CHIP_ERASE         0xC7  // Erase entire chip (~20s)
#define W25Q_CMD_READ_STATUS_1      0x05  // Read status register 1 (WIP, WEL, BP[2:0])
#define W25Q_CMD_READ_STATUS_2      0x35  // Read status register 2 (QE quad enable)
#define W25Q_CMD_WRITE_STATUS       0x01  // Write status register (1+2 bytes)
#define W25Q_CMD_WRITE_ENABLE       0x06  // Set WEL (write enable latch)
#define W25Q_CMD_WRITE_DISABLE      0x04  // Clear WEL
#define W25Q_CMD_DEEP_SLEEP         0xB9  // Enter deep power-down (consume <1µA)
#define W25Q_CMD_RELEASE_SLEEP      0xAB  // Release from deep power-down

// Status register 1 bits
#define W25Q_SR1_WIP                (1 << 0) // Write-in-progress
#define W25Q_SR1_WEL                (1 << 1) // Write enable latch
#define W25Q_SR1_BP0                (1 << 2) // Block protect bit 0
#define W25Q_SR1_BP1                (1 << 3) // Block protect bit 1
#define W25Q_SR1_BP2                (1 << 4) // Block protect bit 2
#define W25Q_SR1_TB                 (1 << 5) // Top/bottom block protect
#define W25Q_SR1_SEC                (1 << 6) // Sector/block protect
#define W25Q_SR1_SRP0               (1 << 7) // Status register protect bit 0

// Status register 2 bits
#define W25Q_SR2_SRP1               (1 << 0) // Status register protect bit 1
#define W25Q_SR2_QE                 (1 << 1) // Quad enable (must set for quad commands)
#define W25Q_SR2_RESERVED           (3 << 2) // Reserved
#define W25Q_SR2_LB0                (1 << 3) // Security register lock bit 0
#define W25Q_SR2_LB1                (1 << 4) // Security register lock bit 1
#define W25Q_SR2_LB2                (1 << 5) // Security register lock bit 2
#define W25Q_SR2_LB3                (1 << 6) // Security register lock bit 3
#define W25Q_SR2_CMP                (1 << 7) // Complement protect

// Session logging allocation (example: 8 weeks of data)
#define FLASH_LOG_BASE_ADDR         0x000000  // Start of session logs (sector 0)
#define FLASH_LOG_MAX_SIZE          (15 * 1024 * 1024) // 15 MB for logs
#define FLASH_CONFIG_BASE_ADDR      (15 * 1024 * 1024) // Last 1 MB for config/calibration
#define FLASH_CONFIG_SIZE           (1 * 1024 * 1024)

// Session structure (written to config area)
typedef struct {
    uint32_t session_id;                // Unique session ID (Unix timestamp of start)
    uint32_t num_shots;                 // Total shots in session
    uint32_t start_address;             // First byte of shot data
    uint32_t end_address;               // Last byte of shot data
    uint64_t start_time_us;             // Start time in microseconds
    uint64_t end_time_us;               // End time in microseconds
    uint8_t checksum;                   // CRC8 of session header
} flash_session_t;

// Shot record structure (20 bytes per shot)
typedef struct {
    uint32_t timestamp_us;              // Relative timestamp from session start
    int16_t accel_x, accel_y, accel_z;  // Acceleration (3 × 2 bytes = 6 bytes)
    int16_t gyro_x, gyro_y, gyro_z;     // Angular velocity (3 × 2 bytes = 6 bytes)
    int16_t barrel_temp_raw;            // Barrel temperature sensor ADC (2 bytes)
    float poi_shift_mm;                 // POI shift due to thermal drift (4 bytes)
    uint32_t crc32;                     // CRC32 of this record (4 bytes)
} flash_shot_record_t; // 20 bytes total

// Function prototypes
int flash_init(void);                   // Initialize QSPI peripheral
int flash_read_jedec_id(uint32_t *id);  // Read JEDEC ID (expect 0xEF4018)
int flash_read_status(uint8_t *sr1, uint8_t *sr2); // Read status registers
int flash_enable_quad_mode(void);       // Set QE bit for quad I/O commands
int flash_wait_ready(void);             // Poll WIP bit until ready (100ms timeout)
int flash_write_enable(void);           // Set WEL bit before write/erase
int flash_read(uint32_t addr, uint8_t *buf, uint32_t len); // Read @ 25 MHz (single I/O)
int flash_fast_read_quad(uint32_t addr, uint8_t *buf, uint32_t len); // Read @ 104 MHz (quad I/O)
int flash_page_write(uint32_t addr, uint8_t *buf, uint32_t len); // Write ≤256 bytes/page
int flash_quad_page_write(uint32_t addr, uint8_t *buf, uint32_t len); // Write @ 104 MHz (quad)
int flash_sector_erase(uint32_t addr);  // Erase 4KB sector (100ms typical)
int flash_block32_erase(uint32_t addr); // Erase 32KB block (800ms typical)
int flash_block64_erase(uint32_t addr); // Erase 64KB block (1s typical)
int flash_chip_erase(void);             // Erase entire chip (~20s, blocking)
int flash_deep_sleep(void);             // Enter deep power-down (<1µA consumption)
int flash_release_sleep(void);          // Wake from deep power-down
int flash_soft_reset(void);             // Reset command (0xFF)

// Session management
int flash_create_session(uint32_t session_id, flash_session_t *session);
int flash_append_shot(uint32_t session_id, flash_shot_record_t *shot);
int flash_read_session(uint32_t session_id, flash_session_t *session);
int flash_list_sessions(uint32_t *session_ids, uint8_t *count, uint8_t max_count);
int flash_erase_session(uint32_t session_id);

// Utility
uint32_t flash_crc32(uint8_t *data, uint32_t len); // CRC32 calculation
uint32_t flash_get_free_space(void);   // Remaining space for logs
float flash_get_utilization(void);     // Percent used (0-100)
```

**Implementation checklist:**
- [ ] QSPI peripheral initialization (PA6/PA7/PA8/PA9 clock/IO/mosi/miso)
- [ ] QSPI clock setup (104 MHz for W25Q128)
- [ ] JEDEC ID read verification (0xEF4018)
- [ ] Status register read/write
- [ ] Quad mode enable (set SR2 QE bit)
- [ ] Write enable/disable flow
- [ ] Page program (single or quad I/O)
- [ ] Sector/block erase with timeout
- [ ] Wait-ready polling (WIP bit)
- [ ] CRC32 calculation for data integrity
- [ ] Session management (create, append, list, erase)
- [ ] Free space tracking
- [ ] Error handling (timeout, CRC mismatch)

---

#### 2.6: AT24C256C I2C EEPROM Driver

**File:** `firmware/src/drivers/eeprom_i2c.c`  
**Status:** ⏳ IMPLEMENT

**Technical Specification (TZ):**
```c
// AT24C256C I2C EEPROM (32 KB, 400 kHz I2C)
#define EEPROM_I2C_ADDR             0x60  // Fixed address (A0/A1/A2 = 0, or configure)
#define EEPROM_TOTAL_SIZE           (32 * 1024) // 32 KB
#define EEPROM_PAGE_SIZE            64    // 64-byte write page
#define EEPROM_WRITE_TIME_MS        5     // Max write cycle time (5ms typical, 10ms max)
#define EEPROM_ACK_POLL_TIMEOUT_MS  100   // Max time to wait for ACK polling

// Memory layout
#define EEPROM_CONFIG_BASE_ADDR     0x0000 // Configuration & calibration (0-4KB)
#define EEPROM_CONFIG_SIZE          (4 * 1024)
#define EEPROM_SESSION_METADATA     0x1000 // Session index (4KB-8KB)
#define EEPROM_SESSION_METADATA_SIZE (4 * 1024)
#define EEPROM_RESERVED             0x2000 // Reserved for future use (8KB+)

// Configuration structures stored in EEPROM
typedef struct {
    uint32_t config_version;            // Config format version (v1.0 = 0x01000000)
    float kalman_q_accel;               // Kalman filter process noise (Q)
    float kalman_r_measure;             // Kalman filter measurement noise (R)
    float anomaly_threshold_recoil;     // Anomaly detection recoil threshold (2.5× baseline)
    float anomaly_threshold_temp;       // Anomaly detection temperature threshold
    uint8_t num_thermal_calib_points;   // Number of thermal calibration points
    uint32_t timestamp_last_calib_us;   // Timestamp of last thermal calibration
    uint8_t checksum_config;            // CRC8 checksum of this structure
} eeprom_config_t;

// Calibration point (thermal drift compensation)
typedef struct {
    float barrel_temp_celsius;          // Barrel temperature (°C)
    float poi_shift_x_mm;               // POI shift X (mm)
    float poi_shift_y_mm;               // POI shift Y (mm)
} eeprom_thermal_calib_point_t;

// Session metadata entry (4 bytes per session, 256 max sessions)
typedef struct {
    uint32_t session_id;                // Session ID or 0xFFFFFFFF if unused
} eeprom_session_entry_t;

// Function prototypes
int eeprom_init(void);                  // Initialize I2C peripheral
int eeprom_read(uint16_t addr, uint8_t *buf, uint16_t len); // Read arbitrary bytes
int eeprom_write(uint16_t addr, uint8_t *buf, uint16_t len); // Write with page alignment
int eeprom_write_byte(uint16_t addr, uint8_t val); // Write single byte
int eeprom_write_page(uint16_t page_addr, uint8_t *buf, uint8_t len); // Write one 64-byte page
int eeprom_wait_ready(void);            // ACK polling until ready (100ms timeout)

// Configuration management
int eeprom_read_config(eeprom_config_t *config);
int eeprom_write_config(eeprom_config_t *config);
int eeprom_read_thermal_calib(uint8_t index, eeprom_thermal_calib_point_t *point);
int eeprom_write_thermal_calib(uint8_t index, eeprom_thermal_calib_point_t *point);

// Session metadata
int eeprom_register_session(uint32_t session_id);
int eeprom_get_session_list(uint32_t *session_ids, uint8_t *count, uint8_t max_count);

// Utility
int eeprom_erase_all(void);             // Clear entire EEPROM (0xFF)
int eeprom_selftest(void);              // Write/read test pattern
uint8_t eeprom_crc8(uint8_t *data, uint16_t len); // CRC8 calculation
```

**Implementation checklist:**
- [ ] I2C initialization (PB10/PB11 @ 400 kHz)
- [ ] ACK polling for write completion
- [ ] Random read (address + data)
- [ ] Sequential read (streaming up to page boundary)
- [ ] Page write (64 bytes at a time)
- [ ] Single byte write with ACK polling
- [ ] CRC8 checksum validation
- [ ] Configuration read/write
- [ ] Thermal calibration point storage
- [ ] Session metadata management
- [ ] Error handling (I2C timeout, CRC mismatch)

---

### ⏳ IMPLEMENT: Power Management & Watchdogs

**Priority:** MEDIUM  
**Estimated effort:** 3 hours

#### 2.7: Independent Watchdog (IWDG) - Hardware Watchdog

**File:** `firmware/src/system/iwdg.c`  
**Status:** ⏳ IMPLEMENT

**Technical Specification (TZ):**
```c
// STM32H745 Independent Watchdog (IWDG)
// Independent LSI oscillator (40 kHz ±10%)
// Max timeout: ~26 seconds
// Key registers:
// - IWDG_KR (Key register): 0xAAAA unlock, 0x5555 reload, 0xCCCC enable
// - IWDG_PR (Prescaler): /4, /8, /16, /32, /64, /128, /256
// - IWDG_RLR (Reload): value (0-4095)

// Timeout calculation: T = RLR / (LSI / prescaler)
// Example: LSI=40kHz, prescaler=32 → timeout = RLR * 800µs
// For 5-second timeout: RLR = 5 / 0.0008 = 6250 (too large, max 4095)
// For 3-second timeout: RLR = 3 / 0.0008 = 3750 ✓

#define IWDG_TIMEOUT_SECONDS        30    // 30-second watchdog for M7 sensor fusion
#define IWDG_KR_UNLOCK              0xAAAA
#define IWDG_KR_RELOAD              0x5555
#define IWDG_KR_START               0xCCCC
#define IWDG_PR_DIV_256             0x06  // Largest prescaler
#define IWDG_RLR_MAX                4095  // Max reload value

// Function prototypes
int iwdg_init(uint8_t timeout_seconds); // Initialize with timeout
void iwdg_kick(void);                   // Feed watchdog (reload counter)
uint16_t iwdg_get_remaining(void);      // Get remaining count before reset
void iwdg_force_reset(void);            // Trigger intentional reset
```

**Implementation checklist:**
- [ ] LSI oscillator frequency verification
- [ ] Prescaler calculation for desired timeout
- [ ] Reload register setup
- [ ] Watchdog enable via KR register
- [ ] Kick/reload routine
- [ ] Remaining time calculation
- [ ] ISR setup (optional early warning)

---

#### 2.8: Window Watchdog (WWDG) - Software Watchdog

**File:** `firmware/src/system/wwdg.c`  
**Status:** ⏳ IMPLEMENT

**Technical Specification (TZ):**
```c
// STM32H745 Window Watchdog (WWDG)
// Uses AHB clock (480 MHz M7, 240 MHz M4)
// Configurable window: counter must be refreshed in allowed range
// Early interrupt generation before reset
// Used for M4 core monitoring (verify task execution)

#define WWDG_TIMEOUT_MS             1000  // 1-second window for M4 heartbeat
#define WWDG_PRESCALER              0x03  // /32: period = 32/480MHz = 66.7ns per tick
#define WWDG_COUNTER_INIT           0x7F  // Start counter at max (127)
#define WWDG_LOWER_LIMIT            0x50  // Lower window limit (~40 at init)
// If counter drops below limit before refresh, reset triggered
// If counter > 0x7F (upper), reset triggered

// Function prototypes
int wwdg_init(uint16_t timeout_ms);     // Initialize M4 watchdog
void wwdg_kick(void);                   // Refresh counter (M4 proves it's alive)
uint8_t wwdg_get_counter(void);         // Get current counter value
void wwdg_enable_interrupt(void);       // Enable early warning interrupt
void wwdg_isr(void);                    // Early warning ISR (log M4 failure)
```

**Implementation checklist:**
- [ ] Prescaler setup for AHB clock
- [ ] Timeout calculation
- [ ] Counter and window register setup
- [ ] Interrupt enable/disable
- [ ] ISR implementation (early warning handler)
- [ ] Recovery strategy (graceful shutdown vs. reset)

---

### ⏳ IMPLEMENT: Data Logging & Session Management

**Priority:** HIGH  
**Estimated effort:** 4 hours

#### 2.9: Session Manager

**File:** `firmware/src/logging/session_manager.c`  
**Status:** ⏳ IMPLEMENT

**Technical Specification (TZ):**
```c
// Session lifecycle management
// - Session creation (start timestamp, unique ID)
// - Shot recording (shot data written to Flash)
// - Session closure (end timestamp, CRC verification)
// - Metadata persistence (EEPROM)

#define MAX_SESSIONS_IN_EEPROM      256   // Max sessions tracked
#define SESSION_ID_BASE_YEAR        2026  // Session ID = (year-2000)*1000 + DOY

typedef struct {
    uint32_t session_id;                // Unique session ID
    uint32_t num_shots;                 // Total shots in session
    uint64_t start_time_us;             // Session start (µs since power-on)
    uint64_t end_time_us;               // Session end
    uint32_t flash_start_addr;          // First shot address in Flash
    uint32_t flash_end_addr;            // Last shot address
    uint32_t flash_session_crc32;       // CRC32 of all shots
    uint8_t session_status;             // 0=active, 1=closed, 2=corrupted
} session_metadata_t;

// Circular buffer for shots (in RAM, written to Flash periodically)
#define SHOT_BUFFER_SIZE            256   // 256-shot buffer before Flash write
typedef struct {
    flash_shot_record_t shots[SHOT_BUFFER_SIZE];
    uint16_t write_ptr;                 // Insert position (0-255)
    uint16_t num_valid;                 // Valid shots in buffer (0-256)
} shot_buffer_t;

// Function prototypes
int session_create(uint32_t *session_id); // Create new session
int session_close(uint32_t session_id);   // Close session (flush buffer, write CRC)
int session_add_shot(uint32_t session_id, flash_shot_record_t *shot); // Add shot to buffer
int session_flush_shots(uint32_t session_id); // Write buffer to Flash
int session_get_metadata(uint32_t session_id, session_metadata_t *meta);
int session_list_all(uint32_t *ids, uint8_t *count, uint8_t max_count);
int session_delete(uint32_t session_id);  // Erase from EEPROM + Flash
uint32_t session_get_shot_count(uint32_t session_id);
```

**Implementation checklist:**
- [ ] Session ID generation (unique, monotonic)
- [ ] EEPROM metadata storage
- [ ] Circular shot buffer management
- [ ] Flash write batching (write 256 shots at a time)
- [ ] CRC32 calculation & verification
- [ ] Session closure & finalization
- [ ] Error recovery (corrupted session detection)

---

### ⏳ IMPLEMENT: Test & Debug Infrastructure

**Priority:** MEDIUM  
**Estimated effort:** 3 hours

#### 2.10: Unit Tests Framework

**File:** `firmware/tests/test_runner.c`  
**Status:** ⏳ IMPLEMENT

**Technical Specification (TZ):**
```c
// Simple unit test framework
// Run on ARM STM32 via UART output
// Assertion macros for pass/fail verification

#define TEST_ASSERT(condition, msg) \
    if (!(condition)) { \
        printf("[FAIL] %s\n", msg); \
        test_failures++; \
    } else { \
        printf("[PASS] %s\n", msg); \
        test_passes++; \
    }

// Test categories
// 1. Kalman filter: verify covariance convergence, state prediction
// 2. Anomaly detector: threshold crossing, hysteresis recovery
// 3. Thermal compensation: outlier rejection, polynomial fitting
// 4. Sensor drivers: data validity, CRC, I2C/SPI communication
// 5. Flash/EEPROM: read/write/erase, wear leveling
// 6. Session management: creation, closure, corruption recovery

// Function prototypes
int test_kalman_filter(void);           // 129 unit tests (existing)
int test_anomaly_detector(void);        // Anomaly detection logic
int test_thermal_robust(void);          // Robust regression
int test_sensor_drivers(void);          // ICM-20689, VL53L0X, etc.
int test_flash_operations(void);        // QSPI Flash driver
int test_eeprom_operations(void);       // I2C EEPROM driver
int test_session_manager(void);         // Session create/close/list

int run_all_tests(void);                // Run all test suites
```

**Implementation checklist:**
- [ ] Test assertion macros
- [ ] Test result logging (UART output)
- [ ] Sensor driver mock data
- [ ] Flash/EEPROM simulation for unit tests
- [ ] Code coverage measurement
- [ ] Test result summary

---

## Phase 3: PCB Layout & Manufacturing

### ⏳ IMPLEMENT: PCB Layout (KiCad)

**Priority:** CRITICAL  
**Estimated effort:** 40 hours (parallel with firmware)

**Status:** ⏳ START KiCad LAYOUT

- [ ] Component placement (all 41 components positioned)
- [ ] Critical net routing (QSPI @ 50Ω impedance)
- [ ] Power distribution (12V → 5V → 3.3V/1.8V)
- [ ] Ground plane optimization
- [ ] Via placement & thermal vias under MCU
- [ ] Design rule check (DRC) — target: zero errors
- [ ] Gerber file generation
- [ ] Impedance verification report

---

### ⏳ IMPLEMENT: Thermal & SI Analysis

**Priority:** HIGH  
**Estimated effort:** 8 hours (can be parallel)

**Status:** ⏳ SI & FEA SIMULATION

- [ ] Signal integrity: QSPI @ 104 MHz timing
- [ ] Thermal FEA: component temperature distribution
- [ ] EMI analysis: clock radiation
- [ ] DFM review with manufacturer

---

## Phase 4: Manufacturing & Bring-Up

### ⏳ IMPLEMENT: Manufacturing Test Program

**Priority:** HIGH  
**Estimated effort:** 6 hours

**File:** `firmware/src/test/mfg_test.c`  
**Status:** ⏳ IMPLEMENT

**Specifications (TZ):**
```c
// Manufacturing acceptance test (PAT)
// Run after assembly, before shipping

#define MFG_TEST_TIMEOUT_MS         10000 // 10-second timeout per test

int mfg_test_power_rails(void);         // VCC_5V, VCC_3V3, VCC_1V8 ±5%
int mfg_test_clock(void);               // 480 MHz M7, 240 MHz M4
int mfg_test_uart(void);                // 921.6k baud loopback
int mfg_test_swd(void);                 // Cortex M7/M4 SWD connection
int mfg_test_imu(void);                 // ICM-20689 JEDEC ID
int mfg_test_rangefinder(void);         // VL53L0X distance measurement
int mfg_test_temperature_sensors(void); // MCP9808 + BMP390 readings
int mfg_test_flash(void);               // W25Q128JV JEDEC ID + read/write
int mfg_test_eeprom(void);              // AT24C256C read/write
int mfg_test_watchdog(void);            // IWDG reset trigger
int mfg_test_thermal(void);             // No component >60°C @ load

int mfg_test_suite_run(void);           // Execute all tests
uint8_t mfg_test_get_pass_count(void);
uint8_t mfg_test_get_fail_count(void);
char* mfg_test_get_results_string(void); // Summary for operator
```

**Implementation checklist:**
- [ ] Test entry point (run from UART command)
- [ ] Individual test functions (pass/fail reporting)
- [ ] Timeout handling
- [ ] Test result logging
- [ ] Go/No-go decision logic
- [ ] Clear operator instructions

---

## 📊 Implementation Status Summary

| Component | Status | Priority | Effort | Target Date |
|-----------|--------|----------|--------|-------------|
| **Firmware Core** | ✅ Complete | - | - | 2026-07-28 |
| Sensor Drivers (4×) | ⏳ 0/4 | HIGH | 4h | 2026-08-04 |
| Memory Drivers (2×) | ⏳ 0/2 | HIGH | 6h | 2026-08-04 |
| Watchdog Drivers (2×) | ⏳ 0/2 | MEDIUM | 3h | 2026-08-05 |
| Session Manager | ⏳ 0/1 | HIGH | 4h | 2026-08-05 |
| Unit Tests | ⏳ 0/6 | MEDIUM | 3h | 2026-08-06 |
| **PCB Layout** | ⏳ 0% | CRITICAL | 40h | 2026-08-11 |
| **SI/FEA Analysis** | ⏳ 0% | HIGH | 8h | 2026-08-10 |
| **Mfg Test Program** | ⏳ 0/1 | HIGH | 6h | 2026-08-15 |

**Total remaining effort:** ~74 hours  
**Critical path:** PCB layout (40 hours, must complete by 2026-08-11)

---

## 🚀 Next Immediate Actions

1. **Start sensor drivers** (firmware/src/drivers/)
   - ICM-20689 (SPI IMU) - 1 hour
   - VL53L0X (I2C LRF) - 1 hour
   - MCP9808 (I2C temp) - 0.5 hour
   - BMP390 (I2C pressure) - 1 hour

2. **Start memory drivers** (firmware/src/drivers/)
   - QSPI Flash (W25Q128JV) - 2 hours
   - I2C EEPROM (AT24C256C) - 1 hour

3. **Parallel: Begin KiCad PCB layout**
   - Component placement
   - Critical QSPI routing
   - Power distribution

4. **Schedule FEA/SI analysis**
   - Book ANSYS or COMSOL time
   - Schedule HyperLynx SI

---

**Document Status:** 🚀 ACTIVE - Implementation in progress  
**Last Updated:** 2026-07-28  
**Next Update:** Daily standups with progress tracking
