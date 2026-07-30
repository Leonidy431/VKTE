cmake_minimum_required(VERSION 3.20)
project(STM32H745_Shotgun_Testbench C ASM)

# ============ Compiler Settings ============
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -O2 -g3")

# ============ Coverage Instrumentation (host tests only) ============
option(ENABLE_COVERAGE "Build host tests with gcov instrumentation" OFF)
if(ENABLE_COVERAGE AND NOT CMAKE_SYSTEM_NAME MATCHES "Generic")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -O0")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

# ============ ARM Toolchain Settings ============
if(CMAKE_SYSTEM_NAME MATCHES "Generic")
    set(CMAKE_C_COMPILER "arm-none-eabi-gcc")
    set(CMAKE_CXX_COMPILER "arm-none-eabi-g++")
    set(CMAKE_ASM_COMPILER "arm-none-eabi-as")
    set(CMAKE_AR "arm-none-eabi-ar")
    set(CMAKE_OBJCOPY "arm-none-eabi-objcopy")
    set(CMAKE_OBJDUMP "arm-none-eabi-objdump")
    set(CMAKE_SIZE "arm-none-eabi-size")

    # STM32H745 specific flags
    set(MCU_FLAGS "-mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MCU_FLAGS}")
endif()

# ============ Project Options ============
option(BUILD_FIRMWARE "Build STM32 firmware" OFF)
option(BUILD_TESTS "Build unit tests (host)" ON)
option(ENABLE_DEBUG "Enable debug output" ON)

# ============ Include Directories ============
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/firmware/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/inc
)

# ============ Test Executables (Host) ============
# Each test links the REAL firmware source file(s) it exercises, never a
# reimplementation, so a passing test suite means the shipped code passed.
if(BUILD_TESTS AND NOT CMAKE_SYSTEM_NAME MATCHES "Generic")
    message(STATUS "Building unit tests for host platform...")

    # Test: Kalman Filter (links firmware/src/data_fusion/kalman_filter.c)
    add_executable(test_kalman_filter
        tests/unit_tests/test_kalman_filter.c
        firmware/src/data_fusion/kalman_filter.c
    )
    target_link_libraries(test_kalman_filter m)

    # Test: Thermal analysis + CRC32
    # (links thermal_analysis.c and session_log.c, source of compute_crc32)
    add_executable(test_thermal_analysis
        tests/unit_tests/test_thermal_analysis.c
        firmware/src/data_fusion/thermal_analysis.c
        firmware/src/storage/session_log.c
    )
    target_link_libraries(test_thermal_analysis m)

    # Test: Streaming shot assembler (links the real state machine)
    add_executable(test_shot_assembler
        tests/unit_tests/test_shot_assembler.c
        firmware/src/data_fusion/shot_assembler.c
    )
    target_link_libraries(test_shot_assembler m)

    # Test: Robust thermal drift fitting (Huber loss / IRLS, pure logic)
    add_executable(test_thermal_robust
        tests/unit_tests/test_thermal_robust.c
        firmware/src/data_fusion/thermal_robust.c
    )
    target_link_libraries(test_thermal_robust m)

    # Test: Real-time anomaly detector (EWMA / 3-sigma, pure logic)
    add_executable(test_anomaly_detector
        tests/unit_tests/test_anomaly_detector.c
        firmware/src/data_fusion/anomaly_detector.c
    )
    target_link_libraries(test_anomaly_detector m)

    # Test: Shot-record ring buffer + CRC32 + Flash flush
    add_executable(test_session_log
        tests/unit_tests/test_session_log.c
        firmware/src/storage/session_log.c
    )
    target_link_libraries(test_session_log m)

    # Test: M7<->M4 IPC ring buffer
    add_executable(test_m4_ipc
        tests/unit_tests/test_m4_ipc.c
        firmware/src/m4_core.c
    )
    target_link_libraries(test_m4_ipc m)

    # Test: Sensor drivers (ICM-20689, VL53L0X, MCP9808, BMP390) via
    # register-level simulators overriding each driver's weak I/O hooks
    add_executable(test_sensor_drivers
        tests/unit_tests/test_sensor_drivers.c
        firmware/src/drivers/icm20689.c
        firmware/src/drivers/vl53l0x.c
        firmware/src/drivers/mcp9808.c
        firmware/src/drivers/bmp390.c
    )
    target_link_libraries(test_sensor_drivers m)

    # Test: Memory drivers (AT24C256C EEPROM, W25Q128JV QSPI Flash)
    add_executable(test_memory_drivers
        tests/unit_tests/test_memory_drivers.c
        firmware/src/drivers/at24c256c.c
        firmware/src/drivers/w25q128jv.c
    )
    target_link_libraries(test_memory_drivers m)

    # Test: IWDG/WWDG watchdogs, via mmap-backed fixed register addresses
    add_executable(test_watchdog
        tests/unit_tests/test_watchdog.c
        firmware/src/drivers/watchdog.c
    )
    target_link_libraries(test_watchdog m)

    # Test: Session manager (circular Flash session storage)
    add_executable(test_session_manager
        tests/unit_tests/test_session_manager.c
        firmware/src/session_manager.c
    )
    target_link_libraries(test_session_manager m)

    # Test: Manufacturing PAT orchestration
    add_executable(test_manufacturing_test
        tests/unit_tests/test_manufacturing_test.c
        firmware/src/manufacturing_test.c
    )
    target_link_libraries(test_manufacturing_test m)

    # Companion to test_manufacturing_test: links manufacturing_test.c
    # WITHOUT overriding the weak pat_test_* subtests, so their own
    # (currently placeholder) bodies get covered too -- the other binary's
    # overrides replace those bodies for its own link, to exercise the
    # orchestrator's PAT_FAIL branches instead. gcov/gcovr merge both.
    add_executable(test_manufacturing_test_defaults
        tests/unit_tests/test_manufacturing_test_defaults.c
        firmware/src/manufacturing_test.c
    )
    target_link_libraries(test_manufacturing_test_defaults m)

    # Companion to test_sensor_drivers/test_memory_drivers/test_session_manager:
    # links the same driver sources WITHOUT overriding their weak low-level
    # I/O hooks, so the hooks' own (currently placeholder) default bodies
    # get covered too -- the other binaries' overrides replace those bodies
    # for their own links, to exercise realistic init/read/write logic
    # instead. gcov/gcovr merge all of them.
    add_executable(test_drivers_defaults
        tests/unit_tests/test_drivers_defaults.c
        firmware/src/drivers/icm20689.c
        firmware/src/drivers/vl53l0x.c
        firmware/src/drivers/mcp9808.c
        firmware/src/drivers/bmp390.c
        firmware/src/drivers/at24c256c.c
        firmware/src/drivers/w25q128jv.c
        firmware/src/session_manager.c
    )
    target_link_libraries(test_drivers_defaults m)

    # Enable CTest
    enable_testing()
    add_test(NAME KalmanFilter COMMAND test_kalman_filter)
    add_test(NAME ThermalAnalysis COMMAND test_thermal_analysis)
    add_test(NAME ShotAssembler COMMAND test_shot_assembler)
    add_test(NAME ThermalRobust COMMAND test_thermal_robust)
    add_test(NAME AnomalyDetector COMMAND test_anomaly_detector)
    add_test(NAME SessionLog COMMAND test_session_log)
    add_test(NAME M4IpcRing COMMAND test_m4_ipc)
    add_test(NAME SensorDrivers COMMAND test_sensor_drivers)
    add_test(NAME MemoryDrivers COMMAND test_memory_drivers)
    add_test(NAME Watchdog COMMAND test_watchdog)
    add_test(NAME SessionManager COMMAND test_session_manager)
    add_test(NAME ManufacturingTest COMMAND test_manufacturing_test)
    add_test(NAME ManufacturingTestDefaults COMMAND test_manufacturing_test_defaults)
    add_test(NAME DriversDefaults COMMAND test_drivers_defaults)

    message(STATUS "Tests enabled. Run 'ctest' or 'make test' to execute.")
endif()

# ============ Firmware Build (ARM Cross-Compile) ============
if(BUILD_FIRMWARE AND CMAKE_SYSTEM_NAME MATCHES "Generic")
    message(STATUS "Building firmware for STM32H745...")

    # Add firmware source files
    add_executable(stm32h745_testbench.elf
        firmware/src/main.c
        firmware/src/system_init.c
        firmware/src/m4_core.c
        firmware/src/data_fusion/kalman_filter.c
        firmware/src/data_fusion/thermal_analysis.c
        firmware/src/data_fusion/thermal_robust.c
        firmware/src/data_fusion/shot_assembler.c
        firmware/src/data_fusion/anomaly_detector.c
        firmware/src/storage/session_log.c
        firmware/startup/startup_stm32h745xx.s
        firmware/startup/startup_stm32h745xx_m4.s
    )

    # Post-build: Generate binary
    add_custom_command(TARGET stm32h745_testbench.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary stm32h745_testbench.elf stm32h745_testbench.bin
        COMMAND ${CMAKE_SIZE} stm32h745_testbench.elf
        COMMENT "Generating firmware binary..."
    )
endif()

# ============ Summary ============
message(STATUS "")
message(STATUS "╔═══════════════════════════════════════════════════════╗")
message(STATUS "║  STM32H745 Shotgun Testbench - Build Configuration   ║")
message(STATUS "╠═══════════════════════════════════════════════════════╣")
message(STATUS "║  Build Tests:   ${BUILD_TESTS}")
message(STATUS "║  Build Firmware: ${BUILD_FIRMWARE}")
message(STATUS "║  Debug Enabled:  ${ENABLE_DEBUG}")
message(STATUS "║  C Standard:     C${CMAKE_C_STANDARD}")
message(STATUS "╚═══════════════════════════════════════════════════════╝")
message(STATUS "")
