# Development Guide — Volumetric Display Backend

Пошаговая инструкция для локального запуска dev-сервера и тестирования API.

## Требования

- Docker & Docker Compose (установлены)
- Python 3.11+ (для локальной разработки)
- curl или Postman (для тестирования API)

## Быстрый старт

### 1. Запустить dev-сервер

```bash
cd /path/to/VKTE
bash dev.sh
```

Или вручную:

```bash
docker compose up --build backend -d
sleep 20
docker compose ps
```

### 2. Проверить здоровье системы

```bash
curl http://localhost:8000/health
```

**Ожидаемый ответ:**
```json
{"status": "healthy"}
```

### 3. Получить полный статус системы

```bash
curl http://localhost:8000/api/v1/status | python3 -m json.tool
```

**Ожидаемый ответ:**
```json
{
  "bubble_generator": {
    "running": false,
    "frequency_hz": 40000,
    "duty_cycle": 0.5,
    "bubble_radius_um": 50.0,
    "resonant_frequency_hz": 32145,
    "acoustic_pressure_pa": 0.0,
    "stability_regime": "off",
    "density_percent": 0.0,
    "stabilizer": "saponin"
  },
  "laser_controller": {
    "mode": "off",
    "power_w": 0.0,
    "power_max_w": 8.0,
    "wavelength_nm": 532,
    "scanning": false,
    "galvo_position_deg": {"x": 0.0, "y": 0.0}
  },
  "renderer": {
    "rendering": false,
    "grid_resolution": 100,
    "frame_rate_fps": 30,
    "frame_count": 0,
    "active_voxels": 0,
    "total_voxels": 1000000
  },
  "timestamp": 1234567890.123
}
```

## Управление подсистемами

### Пузырьки (Bubble Generator)

**Запустить:**
```bash
curl -X POST "http://localhost:8000/api/v1/bubble/start?frequency_hz=40000&duty_cycle=0.5"
```

**Остановить:**
```bash
curl -X POST "http://localhost:8000/api/v1/bubble/stop"
```

### Лазер (Laser Controller)

**Установить мощность (5 Вт):**
```bash
curl -X POST "http://localhost:8000/api/v1/laser/power?power_w=5.0"
```

**Запустить сканирование:**
```bash
curl -X POST "http://localhost:8000/api/v1/laser/scan"
```

### Рендер (Volumetric Renderer)

**Отрендерить сферу:**
```bash
curl -X POST "http://localhost:8000/api/v1/render/object?object_type=sphere&scale=1.0"
```

Поддерживаемые типы: `sphere`, `cube`, `torus`, `mesh_custom`.

## WebSocket для телеметрии

Подключиться к real-time потоку телеметрии:

```bash
# Using websocat (install: cargo install websocat)
websocat ws://localhost:8000/ws/telemetry

# Or using Python
python3 << 'EOF'
import asyncio
import websockets
import json

async def telemetry():
    uri = "ws://localhost:8000/ws/telemetry"
    async with websockets.connect(uri) as websocket:
        while True:
            data = await websocket.recv()
            print(json.dumps(json.loads(data), indent=2))

asyncio.run(telemetry())
EOF
```

## API Documentation

После запуска сервера откройте в браузере:

```
http://localhost:8000/docs
```

Это интерактивная документация Swagger UI с возможностью тестирования всех endpoint'ов.

## Тестирование

### Автоматическое тестирование API

```bash
python3 test_api.py
```

Запустит полный набор тестов всех endpoint'ов.

### Логи

```bash
docker compose logs -f backend
```

Смотреть логи в real-time.

### Отладка контейнера

```bash
docker compose exec backend bash
# Или:
docker compose exec backend python3 -m pytest tests/
```

## Структура проекта (для разработки)

```
backend/
├── app/
│   ├── main.py                     # FastAPI app, routes
│   ├── config.py                   # Configuration (pydantic)
│   └── modules/
│       └── volumetric/
│           ├── bubble_generator.py # Phase 4: Bubble physics
│           ├── laser_controller.py # Phase 4: Laser + galvo control
│           └── volumetric_renderer.py  # Phase 4: Voxel rendering
├── tests/                          # Unit tests (Phase 8 onwards)
├── Dockerfile
└── requirements.txt
```

## Development Workflow

### 1. Сделать изменение в коде

```bash
# Например, в backend/app/modules/volumetric/bubble_generator.py
nano backend/app/modules/volumetric/bubble_generator.py
```

### 2. Перезагрузить контейнер (с автоперезагрузкой)

Поскольку в `docker-compose.yml` указан флаг `reload=true`, сервер автоматически перезагружается при изменении файлов.

```bash
# Мониторить логи
docker compose logs -f backend
```

### 3. Протестировать

```bash
curl http://localhost:8000/health
python3 test_api.py
```

### 4. Закоммитить

```bash
git add backend/
git commit -m "Phase 5: Fix bubble resonance calculation"
git push origin claude/zmz-402-dual-ignition-arv4z5
```

## Следующие фазы

- **Phase 6:** USB/serial drivers для Steminc трансдьюсеров (40 kHz)
- **Phase 7:** Cambridge Tech galvo scanner (XY2-100 protocol)
- **Phase 8:** Integration tests + field validation
- **Phase 9:** Performance optimization, hardening
- **Phase 10:** Documentation, user guides
- **Phase 11:** Production deployment
- **Phase 12:** Postmortem & lessons learned

## Troubleshooting

### `docker: command not found`

Установить Docker Desktop или Docker Engine.

### `Failed to connect to docker API`

Запустить Docker daemon:
```bash
# macOS/Windows: Open Docker Desktop
# Linux:
sudo systemctl start docker
```

### `Port 8000 already in use`

Остановить конфликтующий процесс:
```bash
docker compose down
lsof -i :8000 | grep LISTEN | awk '{print $2}' | xargs kill -9
```

### Backend не запускается

Посмотреть логи:
```bash
docker compose logs backend
```

---

**Версия:** 0.1.0  
**Последнее обновление:** 2026-07-29  
**Статус:** Phase 0-5 (Bootstrap → Prototype) ✅
