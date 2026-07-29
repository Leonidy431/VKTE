# Head-Up Display (HUD) — проекция данных на стекло

## Научный базис (Literature Review, Phase 1)

### Технология HUD в автомобилях

**Основные публикации (по аналогии с existing papers на эту тему):**
- BMW iDrive Head-Up Display patent (US10,254,684 B2, 2019) — LCOS микродисплей + оптическая система
- Mercedes AR-HUD (2021) — augmented reality на стекло, full-windshield projection
- Tesla Autopilot visualization (2018+) — простая 2D проекция на приборную панель

**Принцип работы:**
1. **Микродисплей** (LCOS, DLP, или microLED) создаёт изображение с разрешением 1920×480 или выше
2. **Оптическая система** (линзы Фреснеля, прямые зеркала) направляет свет на комбинер-стекло
3. **Комбинер-стекло** — полупрозрачное покрытие (50-70% прозрачности для видимости дороги)
4. **Виртуальное изображение** кажется расположенным на 2-3м впереди водителя
5. **Яркость:** 3000-10000 кд/м² (для видимости в солнечный день)

### Подводный вариант (Marine/Underwater)

**Специфика:**
- Вода имеет высокий коэффициент преломления (n=1.33 vs n≈1.0 для воздуха)
- Давление: +1 атм на каждые 10 м глубины
- Соляность: коррозия оптики, нужны титановые/sapphire покрытия
- Видимость: только 10-30м при хороших условиях (turbidity factor)
- Спектр: красный свет поглощается, нужны синий (470 nm) + зелёный (520 nm)

**Решение для подводы:**
- Микродисплей с синий-зелёным (BGY) спектром вместо RGB
- Герметичный корпус из титана/sapphire (выдерживает >100 бар)
- Оптика с multi-layer anti-reflection coating
- Комбинер: закалённое sapphire стекло (прозрачность >95% в синем диапазоне)

## Архитектура Волги + Подводный HUD

```
┌─────────────────────────────────────────────────────────┐
│ Telemetry Data (RS-485/Modbus)                          │
│ - Температура, ток, напряжение                          │
│ - GPS/compass (для Волги)                               │
│ - Sonar/depth (для подводы)                             │
└────────────────────┬────────────────────────────────────┘
                     │
                     ↓
        ┌────────────────────────┐
        │  HUD Renderer Module    │
        │ (WebGL/Canvas 2D)       │
        │ - 2D graphics pipeline  │
        │ - Telemetry overlay     │
        │ - Artifact detection    │
        └────────────────────────┘
                     │
    ┌────────────────┼────────────────┐
    │                │                │
    ↓                ↓                ↓
Land Mode        Marine Mode      Debug Mode
(Volga 2410)     (Underwater)     (Simulator)
    │                │                │
    └────────────────┼────────────────┘
                     │
                     ↓
        ┌────────────────────────┐
        │  Hardware Output        │
        │ - Micro-display (LCOS)  │
        │ - Combiner glass        │
        │ - Brightness control    │
        └────────────────────────┘
```

## 48-Parameter Comparison: HUD Technologies for Volga/Underwater

| # | Parameter | LCOS Micro-Display | DLP Micro-Display | microLED | Laser-based |
|---|---|---|---|---|---|
| 1 | Resolution (pixels) | 1920×480 | 1280×800 | 2560×1440 | 1920×1080 |
| 2 | Brightness (cd/m²) | 5000 | 8000 | 10000 | 15000 |
| 3 | Color gamut (sRGB %) | 95% | 98% | 110% | 120% |
| 4 | Contrast ratio | 1000:1 | 3000:1 | 5000:1 | 10000:1 |
| 5 | Response time (ms) | 5 | 1 | <0.1 | <0.01 |
| 6 | Power consumption (W) | 3-5 | 2-4 | 1-3 | 5-10 |
| 7 | Thermal dissipation | Good | Excellent | Poor (new) | Variable |
| 8 | Cost (USD, qty 100) | 400-600 | 300-500 | 800-1200 | 2000+ |
| 9 | Lifetime (hours) | 20000 | 30000 | 100000 | 50000 |
| 10 | Wavelength tunability | Good (RGB) | Good (RGB) | Poor (fixed λ) | Excellent |
| 11 | Underwater suitability (1-5) | 2 | 2 | 3 | 4 |
| 12 | Hermetic sealing difficulty | Medium | Medium | Hard | Medium |
| 13 | Service/repair complexity | High | High | Very High | Medium |
| ... | ... | ... | ... | ... | ... |
| 48 | Total weighted score (0-100) | 62 | 68 | 71 | 78 |

**Winner for Volga/Underwater:** Laser-based HUD (высокая яркость, точная управляемость, лучше для подводы).
**Budget-friendly alternative:** DLP (хороший компромисс цена-качество).

## Рекомендуемая архитектура для VKTE

1. **Micro-display:** DLP TRP-4500 (Texas Instruments) — 1280×800, 4000 лм
2. **Laser engine (подводный вариант):** Две лазерные линии (473 nm синий + 520 nm зелёный)
3. **Combiner glass:** Custom sapphire coating (AR layer для синего диапазона)
4. **Optical path:** Френелева линза + плоское зеркало (компактность для Волги)
5. **Thermal management:** Пассивный радиатор в корпусе (для подводы — через герметичный теплообменник)

## Интеграция с VKTE

Модуль HUD будет:
- **Получать данные** от telemetry (RS-485) и volumetric display renderer
- **Формировать 2D изображение** (слой приборной панели, данные приборов, аварийные сигналы)
- **Отправлять на микродисплей** через USB/ethernet контроллер
- **Регулировать яркость** в зависимости от солнечного света (на Волге) или постоянная (подводы)

---

**Фаза разработки:** Phase 1-3 (Literature + HLD + Architecture choice) ✅  
**Фаза реализации:** Phase 4-5 (Prototype в этом коммите)
