cmake_minimum_required(VERSION 3.20)
project(STM32H745_Shotgun_Testbench C ASM)

# ============ Compiler Settings ============
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -O2 -g3")

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

    # Enable CTest
    enable_testing()
    add_test(NAME KalmanFilter COMMAND test_kalman_filter)
    add_test(NAME ThermalAnalysis COMMAND test_thermal_analysis)
    add_test(NAME ShotAssembler COMMAND test_shot_assembler)

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
