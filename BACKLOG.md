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

## Sprint 3 (Performance & Monitoring) — не сделано

- [ ] C1: Voxel budget management — лимит активных вокселей на кадр в VolumetricRenderer (бюджет из расчёта скорости галво: 500 мкс/точка → ~66 вокселей на кадр при 30 FPS; сейчас грид 100³ рисуется без бюджета)
- [ ] C2: Hilbert curve scan path — планировщик траектории галво (минимизация переездов между вокселями vs наивный порядок)
- [ ] D1: Prometheus metrics — prometheus-client уже в requirements, экспортёр не подключён (/metrics эндпоинт: FPS, латентность, плотность пузырьков, температуры)
- [ ] D1: Real-time dashboard (frontend/src пуст — см. ниже)

## Sprint 4 (Features & Polish) — не сделано

- [ ] Adaptive brightness — автояркость HUD по датчику освещённости (фотодиод на I2C, см. PCB_PINOUT_DESIGN.md)
- [ ] WebSocket reconnect logic — на стороне frontend (когда появится)
- [ ] API versioning framework — сейчас префикс /api/v1 захардкожен; вынести в роутеры
- [ ] Database persistence layer — история телеметрии (SQLite для прототипа, TimescaleDB для прод)

## Аппаратная часть (новое, по переписке)

- [ ] **KiCad: несущая плата (carrier) + разъёмы под выбранные стандартные платы** — начато: `hardware/kicad/` (см. hardware/kicad/README.md)
- [ ] **Корпус: панели и посадка стандартных плат** — ТЗ по 12 фазам: docs/TZ_ENCLOSURE_CONNECTORS.md; фазы 4+ (детальный дизайн панелей, DXF/STEP) — в работе
- [ ] Прошивка thermal throttling (пороги из POWER_THERMAL_DESIGN.md: лазер 60/80°C, TRIAC 65/85°C)
- [ ] Драйверы реального железа: pyserial → Steminc (RS-485/Modbus RTU), XY2-100 → Cambridge 6215H (SPI)

## Инфраструктура — не сделано

- [ ] frontend/ — каталог существует, но пуст (src без файлов): React UI, WebSocket-телеметрия, панель управления
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
**Версия:** 2.0 (актуализация по переписке: аудит, спринты 1–2, тесты 100%, PEP8, старт KiCad)
