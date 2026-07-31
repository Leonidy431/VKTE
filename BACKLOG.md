# Backlog — недоделано по переписке и спекам

**Статус:** Backend закрыт тестами на 100% (304 теста), безопасность и деградация внедрены.
Актуальный фокус: Sprint 3–4 из ARCHITECTURE_AUDIT.md + аппаратная часть (KiCad, корпус, разъёмы).

## Сделано (по переписке, июль 2026)

- [x] Архитектурный аудит + анализ конкурентов (ARCHITECTURE_AUDIT.md)
- [x] Sprint 1: Pydantic-валидация всех входов, Bearer-аутентификация, rate limiting, structured logging (structlog)
- [x] Sprint 2 A2: синхронизация laser/bubble — скан лазера не стартует без активной генерации пузырьков (RenderingCoordinator)
- [x] Sprint 2 A3: graceful degradation — отказ одной подсистемы при старте не роняет приложение; `/health` отдаёт `degraded_subsystems`
- [x] Coverage 100% (цель была 85%+, затем 99%): 304 теста по всем модулям
- [x] Реальные баги, найденные тестами и исправленные:
  - untyped `request` в rate-limited эндпоинтах ломал их все (422 на любой запрос)
  - NaN/Infinity во входе ронял сериализатор ответа 422 → был unhandled 500 (добавлен кастомный обработчик RequestValidationError с санитайзером)
  - несовпадение границ частоты пузырьков: схема разрешала 100 кГц, железо — 80 кГц (выровнено на 80 кГц)
  - WebSocket-телеметрия: вечный цикл не замечал disconnect, close мог зависнуть/бросить (закрыто guard'ом по состоянию сокета)
  - мёртвые validate_finite-валидаторы удалены (Pydantic v2 ge/le уже режет NaN/Inf)
- [x] PEP8: black + flake8 clean (line length 100)
- [x] PCB: распиновка и посадочные места (docs/PCB_PINOUT_DESIGN.md), питание и тепло (docs/POWER_THERMAL_DESIGN.md)
- [x] Исследование по научным статьям: проекция на стекло + кнопочное управление, отбор 3 путей по 48 параметрам (docs/RESEARCH_HUD_GLASS_TOP3.md); Путь A (combiner-HUD) — победитель, 216/240
- [x] Frontend Фаза 4–5: `frontend/src/index.html` — canvas HUD-рендер (land/marine/debug), mirror mode для проекции на комбайнер, кнопочное + клавиатурное управление, вызовы `/api/v1/*` с Bearer
- [x] WebSocket reconnect logic (Sprint 4) — экспоненциальный backoff с джиттером в frontend, статус-индикатор соединения
- [x] KiCad старт: carrier-плата + задняя панель (hardware/kicad/), ТЗ по 12 фазам (docs/TZ_ENCLOSURE_CONNECTORS.md), Фазы 1–3 закрыты, Фаза 4 в работе

## Sprint 3 (Performance & Monitoring) — не сделано

- [ ] C1: Voxel budget management — лимит активных вокселей на кадр в VolumetricRenderer (бюджет из расчёта скорости галво: 500 мкс/точка → ~66 вокселей на кадр при 30 FPS; сейчас грид 100³ рисуется без бюджета)
- [ ] C2: Hilbert curve scan path — планировщик траектории галво (минимизация переездов между вокселями vs наивный порядок)
- [ ] D1: Prometheus metrics — prometheus-client уже в requirements, экспортёр не подключён (/metrics эндпоинт: FPS, латентность, плотность пузырьков, температуры)
- [ ] D1: Real-time dashboard — базовый canvas-дашборд теперь есть во frontend/src/index.html (debug-режим показывает все каналы разом); полноценные графики/история — ещё нет

## Sprint 4 (Features & Polish) — частично сделано

- [x] WebSocket reconnect logic — см. выше
- [ ] Adaptive brightness — автояркость HUD по датчику освещённости (фотодиод на I2C, разъём J12 в hardware/kicad/vkte_carrier уже заложен, прошивки/эндпоинта пока нет)
- [ ] API versioning framework — сейчас префикс /api/v1 захардкожен; вынести в роутеры
- [ ] Database persistence layer — история телеметрии (SQLite для прототипа, TimescaleDB для прод)

## Аппаратная часть (новое, по переписке)

- [x] **KiCad: несущая плата (carrier) + разъёмы под выбранные стандартные платы** — rev 0.1 в `hardware/kicad/` (см. hardware/kicad/README.md); официальные футпринты DF40/LVDS — Фаза 4 продолжение
- [ ] **Корпус: панели и посадка стандартных плат** — ТЗ по 12 фазам: docs/TZ_ENCLOSURE_CONNECTORS.md; задняя панель rev 0.1 готова, передняя панель и 3D-корпус — Фаза 4 продолжение
- [ ] Прошивка thermal throttling (пороги из POWER_THERMAL_DESIGN.md: лазер 60/80°C, TRIAC 65/85°C)
- [ ] Драйверы реального железа: pyserial → Steminc (RS-485/Modbus RTU), XY2-100 → Cambridge 6215H (SPI)

## Инфраструктура — не сделано

- [x] frontend/ — базовый combiner-HUD клиент есть (frontend/src/index.html); React/сборка — не потребовались (single-file осознанный выбор, см. frontend/README.md)
- [ ] Физический кнопочный пульт (USB/BT HID) вместо клавиатурных биндингов — hardware TODO
- [ ] Kiosk-режим автозапуска на CM4 (systemd + chromium --kiosk)
- [ ] GitHub Actions CI: pytest + coverage gate (85% минимум; фактически держим 100%) + black --check + flake8
- [ ] Docker image для production (multi-stage, non-root)
- [ ] SECURITY: сменить дефолтный API_KEY через env перед любым деплоем (sk-vkte-dev-change-in-production)

## Долгосрочно (Phase 10–12, из аудита)

- [ ] Multi-instance cluster (Redis sync)
- [ ] ML-тюнинг параметров (размер пузырька, мощность лазера)
- [ ] Predictive maintenance (прогноз деградации компонентов)
- [ ] Внешние API: погода, GPS, сонар-fusion

---

**Создано:** 2026-07-29
**Обновлено:** 2026-07-31
**Версия:** 2.1 (+ исследование HUD-проекции/кнопок, frontend combiner-HUD с WS-реконнектом, KiCad rev 0.1)
