# Backlog — недоделано по переписке и спекам

**Статус:** Требуется реализация код-фреймворка для Volumetric Display проекта.

## Не сделано (по спекам в репо)

### Фаза 0–2: Project Bootstrap (КРИТИЧНО)
- [x] Создать структуру `backend/` (Python FastAPI)
- [ ] Создать структуру `frontend/` (React/Web UI для рендеринга)
- [x] Создать `bubble_generator.py` (модуль генерации пузырьков, acoustic cavitation logic)
- [x] Создать `laser_controller.py` (управление лазером, galvo scanner control)
- [x] Создать `volumetric_renderer.py` (3D voxel rendering engine)
- [x] Создать Docker-compose для local dev (laser simulator, bubble rig mock)
- [ ] Создать GitHub Actions CI/CD pipeline

### Фаза 3–5: Core Algorithm Implementation
- [ ] Реализовать bubble resonance frequency calculator (из BUBBLE_TECHNOLOGY.md)
- [ ] Реализовать acoustic cavitation model (Rayleigh–Plesset equation)
- [ ] Реализовать voxel-to-bubble mapping (resolution: 100×100×100)
- [ ] Реализовать laser scanning trajectory planner (XY galvo control)
- [ ] Тесты для bubble stability (unit tests + simulation)
- [ ] Тесты для laser timing/sync (laser pulse vs bubble position)

### Фаза 6–8: Integration & Validation
- [ ] USB/serial interface для Steminc ultrasonic drivers (40 kHz)
- [ ] USB/ethernet interface для Cambridge Tech galvo scanner (XY2-100 protocol)
- [ ] Real-time telemetry logging (laser power, bubble density, voxel update rate)
- [ ] Mock hardware simulation для testing без реального оборудования
- [ ] Valve test suite (unit, integration, E2E)
- [ ] Documentation: API specification, HW interface protocol, calibration guide

### Фаза 9–11: Deploy & Hardening
- [ ] Docker image для production deployment (minimal, optimized)
- [ ] Health checks & monitoring (Prometheus metrics)
- [ ] Graceful shutdown & error recovery
- [ ] Logging & debugging interface
- [ ] Performance benchmarking (FPS, latency, power consumption)

### Фаза 12: Feedback & Iteration
- [ ] Field test report (first prototype run)
- [ ] Lessons learned (what worked, what failed, why)
- [ ] Updated algorithm parameters (bubble size, laser power, scan frequency)
- [ ] Next iteration planning

---

## Очередь по приоритету (TODO в порядке исполнения)

1. **Backend skeleton** (FastAPI app structure, dependencies, logging)
2. **Bubble generator logic** (core algorithm: frequency, amplitude, duty cycle)
3. **Laser controller** (galvo scanner interface, TTL modulation)
4. **Render engine** (voxel grid, 3D-to-2D laser path conversion)
5. **HW drivers** (USB/serial to Steminc boards, Cambridge galvo controller)
6. **Frontend** (web UI for real-time visualization, parameter tuning)
7. **Docker & CI/CD** (automated builds, tests, deployment)
8. **Tests & validation** (unit, integration, simulation)
9. **Documentation** (API docs, hardware protocol, deployment guide)
10. **Postmortem** (review and lessons learned)

---

**Создано:** 2026-07-29  
**Версия:** 1.0 (Initial backlog from VOLUMETRIC_DISPLAY_SPECIFICATION.md)
