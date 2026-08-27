# Долги оператору — VKTE STM32H745 v1.0 (2026-08-18)
## Критический путь к 2026-08-21 Assembly Gate

**Всего долгов**: 16 P0 + 8 P1 + 4 P2 (28 total)  
**Остаток до deadline**: 3 дня 2 часа  
**Статус**: 🔴 КРИТИЧНО — требуется немедленное действие  

---

## 🔴 P0 — MUST FIX ДО 2026-08-20 23:59 UTC (Критичные блокеры)

### Код (Firmware)

| № | Задача | Описание | Владелец | Статус | ETA | Блокирует |
|---|--------|---------|----------|--------|-----|-----------|
| **OP-1** | **Write IPC ring buffer** | `src/ipc_ring_buffer.c` (256-slot FIFO, HSEM spin-lock, enqueue/dequeue, wrap-around) | Guru | 🔴 NEW | 2026-08-19 18:00 | Test #7 (100 shots FIFO) |
| **OP-2** | **Write session_manager.c** | Flash NAND persistence, CRC32 validation, rollback on corruption, 20-shot brownout recovery | Guru | 🔴 NEW | 2026-08-19 20:00 | Test #8 (brownout recovery) |
| **OP-3** | **Write latency_monitor.c** | DWT profiling, GPIO toggle @ ADC sample, p99 histogram, <5ms budget | Guru | 🔴 NEW | 2026-08-20 12:00 | Test #9–10 (latency/throughput) |
| **OP-4** | **Unit test coverage >95%** | test_ipc_ring.c (20 tests: wrap, race, stress) + test_session_manager.c (30 tests: CRC, rollback, corruption) + test_latency_monitor.c (10 tests: DWT accuracy, outliers) | Guru | 🟡 70% | 2026-08-20 18:00 | CI/CD gate before hardware test |

### Build & Infrastructure

| № | Задача | Описание | Владелец | Статус | ETA | Блокирует |
|---|--------|---------|----------|--------|-----|-----------|
| **OP-5** | **Update CMakeLists.txt** | Add targets: ipc_ring_buffer, session_manager, latency_monitor; enable -Wall -Wextra -Wpedantic; link FreeRTOS, HAL | Guru | 🟡 PARTIAL | 2026-08-19 10:00 | Compilation gate |
| **OP-6** | **Create CI/CD pipeline** | `.github/workflows/ci.yml` — lint (clang-format), compile (arm-none-eabi), test (pytest), coverage (gcovr >95%) | Guru | 🔴 NEW | 2026-08-20 10:00 | Automated validation |
| **OP-7** | **Provide arm-none-eabi-gcc** | Cross-compiler for STM32H745 (version 13.2.0+) — required for firmware build | Operator | ⚠️ BLOCKED | 2026-08-19 08:00 | Compilation gate |
| **OP-8** | **Provide OpenOCD config** | `openocd.cfg` for STM32H745 JTAG debugging (interface, target, flash params) | Operator | ⚠️ BLOCKED | 2026-08-19 12:00 | Hardware debugging |

### Hardware Validation Prep

| № | Задача | Описание | Владелец | Статус | ETA | Блокирует |
|---|--------|---------|----------|--------|-----|-----------|
| **OP-9** | **Prepare test rig setup** | (1) Adjustable PSU 2.5–3.6V @ 2A (for brownout tests #2–6) | Operator | ⚠️ TBD | 2026-08-21 06:00 | Test #2–6 execution |
| **OP-10** | **(continued) Test rig setup** | (2) Oscilloscope with 1Ω current sense probe (for inrush measurement) | Operator | ⚠️ TBD | 2026-08-21 06:00 | Test #2 execution |
| **OP-11** | **(continued) Test rig setup** | (3) Logic analyzer for NRST pulse timing, DWT cycle counting | Operator | ⚠️ TBD | 2026-08-21 06:00 | Test #3–4, 9–10 |
| **OP-12** | **(continued) Test rig setup** | (4) Thermal chamber or heat gun for 85°C/95°C validation | Operator | ⚠️ TBD | 2026-08-21 06:00 | Test #5–6 execution |
| **OP-13** | **Verify hardware prototype exists** | PCB assembled, M7/M4 cores populated, NRST/UART2/ADC traces routed, HSEM/IPC mailbox wired | Operator | ⚠️ TBD | 2026-08-21 06:00 | All tests gate |

### Documentation (Critical for test execution)

| № | Задача | Описание | Владелец | Статус | ETA | Блокирует |
|---|--------|---------|----------|--------|-----|-----------|
| **OP-14** | **Expand FIRMWARE_INTEGRATION_TEST_PLAN.md** | Add for each test: (a) step-by-step setup, (b) equipment checklist, (c) expected console output, (d) PASS/FAIL criteria | Guru | 🟡 70% | 2026-08-20 16:00 | Test execution |
| **OP-15** | **Add memory layout diagram** | FLASH: 0x00000000–0x00200000 (2MB), SRAM: 0x24000000–0x24080000 (512KB AXI+DTCM), IPC ring @ 0x2007C000, session data @ 0x08100000 | Guru | 🔴 NEW | 2026-08-19 14:00 | session_manager integration |
| **OP-16** | **Provide STM32H745 Datasheet** | RM0399 (reference manual) — required for HSEM addresses, FLASH controller timing, ADC calibration | Operator | ⚠️ BLOCKED | 2026-08-19 08:00 | session_manager/IPC coding |

---

## 🟠 P1 — SHOULD FIX ДО 2026-08-22 (Высокий приоритет)

| № | Задача | Описание | Владелец | Статус | ETA | Блокирует |
|---|--------|---------|----------|--------|-----|-----------|
| **OP-17** | **Provision Gerber output** | PCB manufacturing files (for assembly & RF validation before high-volume run) | Operator | ⚠️ TBD | 2026-08-20 | PCB assembly order |
| **OP-18** | **Document RF test procedures** | S11 (impedance matching), S21 (crosstalk <−60dB), TX pulse timing, spurious emissions | Guru | ⚠️ PLANNED | 2026-08-21 | RF certification |
| **OP-19** | **Create DevContainer** | `.devcontainer/Dockerfile` with arm-none-eabi-gcc, cmake, openocd, pytest — for reproducible builds | Guru | 🔴 NEW | 2026-08-21 | CI/CD reproducibility |
| **OP-20** | **Write integration diagram** | ASCII/Mermaid: M7 (boot) → clock init → IPC ring enqueue → M4 (dequeue) → Flash batch write → loop | Guru | 🔴 NEW | 2026-08-21 | Architecture clarity |
| **OP-21** | **Update backlog** | IMPLEMENTATION_BACKLOG.md: phases 1–5 with owners, ETAs, blocking dependencies | Guru | ⚠️ PARTIAL | 2026-08-21 | Project transparency |
| **OP-22** | **Provision KiCad project files** | Latest `.kicad_pcb`, `.kicad_sch` after step 8 DRC/ERC (currently at commit 8b43a59) | Operator | ⚠️ TBD | 2026-08-20 | RF validation prep |
| **OP-23** | **Schedule RF lab validation** | Book calibrated RF equipment for S11/S21/emissions testing (3–4 hours) | Operator | ⚠️ TBD | 2026-08-22 | Production readiness |
| **OP-24** | **Provision 4-printer FDM build** | Coordinate PRINT_SCHEDULE_4PRINTERS.md (48–60h completion by 2026-08-20 18:00) with fabrication facility | Operator | ⚠️ TBD | 2026-08-20 | Enclosure assembly |

---

## 🟡 P2 — NICE-TO-HAVE (Post-2026-08-21)

| № | Задача | Описание | Владелец | Статус | ETA |
|---|--------|---------|----------|--------|-----|
| **OP-25** | **Write developer quick-start guide** | Installation, build, test, deploy workflow for future team members | Guru | 🔴 NEW | 2026-08-30 |
| **OP-26** | **Set up automated coverage reporting** | Publish gcovr HTML report to GH Pages on each commit | Guru | 🔴 NEW | 2026-08-30 |
| **OP-27** | **Provision Slack CI notifications** | GitHub → Slack for build failures, coverage drops | Operator | 🔴 NEW | 2026-08-30 |
| **OP-28** | **Plan Phase 6+ architecture** | Sensor fusion (IMU+LRF+Doppler), mesh time-sync, edge deployment roadmap | Guru | 🔴 NEW | 2026-08-30 |

---

## 📊 Риски и mitigation

| Риск | Вероятность | Влияние | Mitigation |
|------|------------|---------|-----------|
| arm-none-eabi-gcc недоступен | 🔴 HIGH | 🔴 BLOCKS ALL | OP-7: Оператор должен предоставить к 2026-08-19 08:00 |
| IPC ring buffer имеет race condition | 🟡 MEDIUM | 🔴 BLOCKS TEST#7 | OP-1: Guru напишет с spin-lock (HSEM) + 20 stress-tests |
| Flash corruption recovery не протестирована | 🟡 MEDIUM | 🔴 BLOCKS TEST#8 | OP-2: CRC32 redundant header + rollback тесты |
| Hardware prototype не готов | 🔴 HIGH | 🔴 BLOCKS ALL | OP-13: Оператор должен подтвердить наличие PCB+компонентов |
| DWT profiling занимает >10% CPU | 🟡 MEDIUM | 🟡 IMPACT | OP-3: Optional toggle for production builds |

---

## 🔄 3-часовые напоминания (Автоматические)

**Первое напоминание**: 2026-08-18 T+3h (примерно 2026-08-18 21:00 UTC)  
**Повтор**: Каждые 3 часа до 2026-08-21 18:00 UTC  
**Получатель**: Сессия Claude Code (этот чат)  

**Шаблон напоминания:**
```
[3h REMINDER] VKTE STM32H745 Critical Path

⏱️ Остаток до 2026-08-21 Assembly: [XYZ часов]

🔴 P0 Progress (MUST COMPLETE):
- OP-1 IPC ring buffer: [STATUS]
- OP-2 session_manager: [STATUS]
- OP-3 latency_monitor: [STATUS]
- OP-4 Unit tests (>95%): [STATUS]
- OP-7 arm-none-eabi-gcc: [REQUIRED BY OPERATOR]
- OP-13 Hardware prototype: [REQUIRED BY OPERATOR]

📋 Next 3-hour focus:
[Guru priority for next checkpoint]

💬 Operator action items blocking progress:
[If any OP-5 through OP-16 are blocking]
```

---

## ✅ Success Criteria (2026-08-21 18:00 UTC)

All P0 items must be ✅ DONE:
- [ ] OP-1: IPC ring buffer passes all 20 tests
- [ ] OP-2: session_manager passes all 30 tests
- [ ] OP-3: latency_monitor passes all 10 tests
- [ ] OP-4: Overall unit test coverage ≥95%
- [ ] OP-5: CMakeLists.txt compiles cleanly
- [ ] OP-6: CI/CD pipeline green on all commits
- [ ] OP-9–12: Test rig fully prepared & calibrated
- [ ] OP-13: Hardware prototype ready (PCB + cores + wiring)
- [ ] OP-14–16: Documentation complete & verified

**Decision**: If all P0 ✅ → **PROCEED TO ASSEMBLY 2026-08-21 18:00**  
If any P0 🔴 → **BLOCK & ESCALATE** (critical path failure)

---

**Подготовлено**: 2026-08-18 T19:23 UTC  
**Оператор**: Требуется немедленное подтверждение получения и действие на OP-7, OP-13  
**Next review**: 2026-08-18 T22:23 UTC (3-часовое напоминание #1)
