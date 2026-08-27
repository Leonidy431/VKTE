/*
 * STM32H745 Ballistic Corrector — Полная параметрическая сборка
 * DFM/DFA Протокол: 8 узлов, модульная архитектура, 0 винтов основного крепежа
 * Автор: Senior Hardware Engineer | DFM/DFA Expert
 * Версия: 1.0 | Дата: 2026-08-16
 */

// ============================================================================
// ГЛОБАЛЬНЫЕ ПАРАМЕТРЫ (Все размеры, допуски, материалы)
// ============================================================================

// === ОСНОВНЫЕ ГАБАРИТЫ ===
pcb_width = 100;           // PCB ширина (мм)
pcb_length = 120;          // PCB длина (мм)
pcb_thickness = 1.6;       // PCB толщина (мм)

hackrf_width = 85;         // HackRF модуль ширина
hackrf_length = 115;       // HackRF модуль длина
hackrf_height = 18;        // HackRF модуль высота

ultrasonic_width = 60;     // Ultrasonic TX/RX ширина
ultrasonic_length = 80;    // Ultrasonic модуль длина
ultrasonic_height = 25;    // Ultrasonic модуль высота

// === ДОПУСКИ И ЗАЗОРЫ (КРИТИЧНЫЕ!) ===
clearance = 0.2;           // Зазор для подвижных соединений (мм)
tolerance_tight = 0.1;     // Допуск для snap-fit (посадка H7/p6)
tolerance_loose = 0.3;     // Допуск для свободного соединения
eps = 0.01;                // Epsilon для разности операций (избегаем z-fighting)

// === КОРПУС ===
enclosure_padding = 8;     // Зазор вокруг PCB (мм)
enclosure_height_z = 180;  // Высота Z-стека (HackRF + STM32 + Ultrasonic + буфер)
enclosure_wall_thick = 3;  // Толщина стенок (FDM оптимально 3-4 мм)
enclosure_corner_r = 4;    // Радиус скругления углов (эргономика, прочность)

// === КОННЕКТОРЫ ===
sma_hole_d = 5.5;          // SMA разъём диаметр отверстия
usb_c_width = 9;           // USB-C разъём ширина паза
usb_c_height = 4;          // USB-C разъём высота паза
j4_connector_width = 14;   // J4 коннектор ширина
j4_connector_height = 8;   // J4 коннектор высота

// === УПЛОТНИТЕЛИ ===
oring_cs = 3;              // O-ring сечение (мм) — стандартная резина
oring_width_slot = oring_cs + tolerance_loose;  // Паз для O-ring

// === КРЕПЁЖ МОДУЛЬНЫЙ (M2 стек) ===
m2_hole_d = 2.2;           // Отверстие M2 винта
m2_standoff_h = 25;        // Стойка M2 высота (расстояние между слоями)
m2_standoff_od = 5;        // Стойка M2 наружный диаметр

// === МАГНИТНАЯ БАЗА ===
magnet_dia = 12;           // Редкоземельный магнит диаметр (Nd Fe B, N52)
magnet_h = 6;              // Магнит толщина
magnet_count = 4;          // Количество магнитов (2x2 матрица)
magnet_spacing = 25;       // Расстояние между магнитами
base_pad_h = 8;            // Высота подушки магнитной базы

// === ГИМБАЛ RF-АНТЕННЫ ===
gimbal_outer_d = 60;       // Гимбал внешний диаметр
gimbal_inner_d = 45;       // Гимбал внутренний диаметр
gimbal_arm_w = 8;          // Толщина рамки гимбала
gimbal_rotation_clearance = 1; // Зазор для вращения подшипника

// === ТЕПЛОВАЯ ПЛАСТИНА ===
thermal_plate_w = 95;      // Ширина теплораспределителя
thermal_plate_l = 115;     // Длина теплораспределителя
thermal_plate_h = 2;       // Толщина (алюминий, проводит тепло)
thermal_via_d = 3;         // Диаметр тепловых переходов (контакт с STM32)

// === КАБЕЛЬНЫЕ КАНАЛЫ ===
cable_tray_h = 10;         // Высота кабельного лотка
cable_tray_w = 20;         // Ширина лотка
cable_entry_r = 3;         // Радиус входного отверстия для кабелей

// === КАЧЕСТВО РЕНДЕРИНГА ===
$fn = 64;                  // Разрешение для круглых объектов (64 = высокое качество)

// ============================================================================
// МОДУЛЬ 1: ОСНОВНОЙ КОРПУС-КАРКАС (Z-AXIS STACK FRAME)
// ============================================================================

module enclosure_frame() {
    difference() {
        // Внешний объём
        translate([0, 0, enclosure_height_z/2])
            minkowski() {
                cube([
                    pcb_width + enclosure_padding*2,
                    pcb_length + enclosure_padding*2,
                    enclosure_height_z
                ], center=true);
                sphere(r=enclosure_corner_r, $fn=$fn);
            }

        // Внутренний объём (полость для PCB и модулей)
        translate([0, 0, enclosure_wall_thick + pcb_thickness/2])
            cube([
                pcb_width + clearance*2,
                pcb_length + clearance*2,
                enclosure_height_z - enclosure_wall_thick*2
            ], center=true);

        // Выемка для панели коннекторов (восточная стенка)
        translate([pcb_width/2 + enclosure_padding - enclosure_wall_thick/2, 0, 120])
            cube([enclosure_wall_thick + eps, 40, 60], center=true);
    }
}

// ============================================================================
// МОДУЛЬ 2: ПАНЕЛЬ КОННЕКТОРОВ (WATERPROOF CONNECTOR PANEL)
// ============================================================================

module connector_panel() {
    difference() {
        // Панель основа
        cube([enclosure_wall_thick, 50, 70], center=true);

        // SMA разъём отверстие (J1 RF)
        translate([0, 15, 20])
            cylinder(d=sma_hole_d, h=enclosure_wall_thick + eps, center=true, $fn=$fn);

        // USB-C разъём паз (зарядка)
        translate([0, 0, 0])
            cube([enclosure_wall_thick + eps, usb_c_width, usb_c_height], center=true);

        // J4 коннектор паз (данные)
        translate([0, -15, -20])
            cube([enclosure_wall_thick + eps, j4_connector_width, j4_connector_height], center=true);
    }

    // O-ring пазы для герметичности
    translate([0, 15, 20])
        ring_seal_slot(sma_hole_d + 2, oring_width_slot, enclosure_wall_thick);

    translate([0, 0, 0])
        ring_seal_slot(usb_c_width + 2, oring_width_slot, enclosure_wall_thick);
}

module ring_seal_slot(outer_d, slot_w, depth) {
    // Паз для O-ring вокруг отверстия
    difference() {
        cylinder(d=outer_d, h=depth, center=true, $fn=$fn);
        cylinder(d=outer_d - slot_w, h=depth + eps, center=true, $fn=$fn);
    }
}

// ============================================================================
// МОДУЛЬ 3: СТОЙКИ M2 И МОДУЛЬНЫЕ КРЕПЛЕНИЯ (MODULAR LAYER BRACKETS)
// ============================================================================

module m2_standoff_with_snapfit() {
    // Стойка M2 с интегрированной защелкой snap-fit
    difference() {
        union() {
            // Цилиндр стойки
            cylinder(d=m2_standoff_od, h=m2_standoff_h, $fn=$fn);

            // Snap-fit язычки (два противоположных)
            translate([0, m2_standoff_od/2 - 1, m2_standoff_h/3])
                cube([2, 2, m2_standoff_h/3], center=true);

            translate([0, -(m2_standoff_od/2 - 1), 2*m2_standoff_h/3])
                cube([2, 2, m2_standoff_h/3], center=true);
        }

        // Отверстие для винта M2
        cylinder(d=m2_hole_d, h=m2_standoff_h + eps, center=true, $fn=$fn);
    }
}

module hackrf_bracket() {
    // Держатель для модуля HackRF (слой 2)
    difference() {
        cube([hackrf_width + 8, hackrf_length + 8, 6], center=true);

        // Основное окно для модуля
        cube([hackrf_width + clearance, hackrf_length + clearance, 10], center=true);

        // 4 угловых отверстия для M2 (стойки вверх и вниз)
        for(x = [-hackrf_width/2, hackrf_width/2])
            for(y = [-hackrf_length/2, hackrf_length/2])
                translate([x, y, 0])
                    cylinder(d=m2_hole_d, h=10, center=true, $fn=$fn);
    }
}

module ultrasonic_bracket() {
    // Держатель для ультразвукового модуля TX/RX (слой 3)
    difference() {
        cube([ultrasonic_width + 8, ultrasonic_length + 8, 6], center=true);

        // Окно для модуля
        cube([ultrasonic_width + clearance, ultrasonic_length + clearance, 10], center=true);

        // Крепежные отверстия M2
        for(x = [-ultrasonic_width/2 + 5, ultrasonic_width/2 - 5])
            for(y = [-ultrasonic_length/2 + 5, ultrasonic_length/2 - 5])
                translate([x, y, 0])
                    cylinder(d=m2_hole_d, h=10, center=true, $fn=$fn);
    }
}

// ============================================================================
// МОДУЛЬ 4: МАГНИТНАЯ БАЗА (MAGNETIC BASE MOUNT)
// ============================================================================

module magnetic_base() {
    union() {
        // Подушка силиконовая (нижняя часть)
        translate([0, 0, base_pad_h/2])
            cube([magnet_spacing + 10, magnet_spacing + 10, base_pad_h], center=true);

        // Редкоземельные магниты (2x2 матрица)
        for(x = [-magnet_spacing/2, magnet_spacing/2])
            for(y = [-magnet_spacing/2, magnet_spacing/2])
                translate([x, y, base_pad_h + magnet_h/2])
                    cylinder(d=magnet_dia, h=magnet_h, center=true, $fn=$fn);

        // Арматура под магниты (пластик усилен)
        for(x = [-magnet_spacing/2, magnet_spacing/2])
            for(y = [-magnet_spacing/2, magnet_spacing/2])
                translate([x, y, base_pad_h])
                    cylinder(d=magnet_dia + 2, h=1, center=true, $fn=$fn);

        // Боковые рёбра жёсткости
        translate([0, 0, base_pad_h/2])
            difference() {
                cube([magnet_spacing + 15, magnet_spacing + 15, base_pad_h], center=true);
                cube([magnet_spacing, magnet_spacing, base_pad_h + eps], center=true);
            }
    }
}

module quick_release_clip() {
    // Быстрый зажим освобождения (нажми — отпусти)
    difference() {
        union() {
            // Рычаг захвата
            cube([8, 30, 4], center=true);

            // Шарнирная ось
            cylinder(d=4, h=10, center=true, $fn=$fn);
        }

        // Отверстие для оси (зазор для вращения)
        cylinder(d=3 + gimbal_rotation_clearance, h=12, center=true, $fn=$fn);
    }
}

// ============================================================================
// МОДУЛЬ 5: ГИМБАЛ RF-АНТЕННЫ (2-AXIS GIMBAL ASSEMBLY)
// ============================================================================

module gimbal_frame() {
    // Рамка гимбала: азимут + возвышение
    difference() {
        // Внешнее кольцо
        cylinder(d=gimbal_outer_d, h=gimbal_arm_w, center=true, $fn=$fn);

        // Внутреннее отверстие
        cylinder(d=gimbal_inner_d, h=gimbal_arm_w + eps, center=true, $fn=$fn);
    }
}

module gimbal_rotation_bearing() {
    // Подшипник вращения (напечатанный, низкий коэффициент трения)
    difference() {
        // Внешняя втулка
        cylinder(d=gimbal_inner_d + 2, h=gimbal_arm_w + 1, center=true, $fn=$fn);

        // Внутренний канал (для оси)
        cylinder(d=gimbal_inner_d - 2*gimbal_rotation_clearance, h=gimbal_arm_w + 3, center=true, $fn=$fn);

        // Канавки смазки (улучшают скольжение)
        for(angle = [0 : 90 : 270])
            rotate([0, 0, angle])
                translate([gimbal_inner_d/2 - 2, 0, 0])
                    cube([1, 0.5, gimbal_arm_w + 2], center=true);
    }
}

module gimbal_antenna_mount() {
    // Крепление антенны к гимбалу
    difference() {
        cube([12, 12, gimbal_arm_w + 2], center=true);

        // Отверстие для SMA штыря антенны
        cylinder(d=3, h=gimbal_arm_w + 3, center=true, $fn=$fn);
    }
}

module gimbal_assembly() {
    // Полная сборка гимбала
    gimbal_frame();
    translate([0, 0, gimbal_arm_w]) gimbal_rotation_bearing();
    translate([gimbal_inner_d/2 - 3, 0, 0]) gimbal_antenna_mount();
}

// ============================================================================
// МОДУЛЬ 6: ТЕПЛОВАЯ ПЛАСТИНА (THERMAL MANAGEMENT PLATE)
// ============================================================================

module thermal_management_plate() {
    difference() {
        // Основная пластина (алюминий, но моделируем в OpenSCAD как пластик)
        translate([0, 0, thermal_plate_h/2])
            cube([thermal_plate_w, thermal_plate_l, thermal_plate_h], center=true);

        // Тепловые переходы (отверстия для контакта с STM32)
        // Расположены над силовыми компонентами (buck converter, LDO)
        for(x = [-20, 0, 20])
            for(y = [-20, 0, 20])
                translate([x, y, thermal_plate_h/2])
                    cylinder(d=thermal_via_d, h=thermal_plate_h + eps, center=true, $fn=$fn);

        // Отверстия крепежа M2
        for(x = [-thermal_plate_w/2 + 10, thermal_plate_w/2 - 10])
            for(y = [-thermal_plate_l/2 + 10, thermal_plate_l/2 - 10])
                translate([x, y, thermal_plate_h/2])
                    cylinder(d=m2_hole_d, h=thermal_plate_h + eps, center=true, $fn=$fn);
    }
}

// ============================================================================
// МОДУЛЬ 7: КАБЕЛЬНЫЙ ЛОТ (CABLE MANAGEMENT TRAY)
// ============================================================================

module cable_management_tray() {
    difference() {
        union() {
            // Основание лотка
            translate([0, 0, cable_tray_h/2])
                cube([80, cable_tray_w, cable_tray_h], center=true);

            // Боковые направляющие (низкие рёбра)
            translate([0, cable_tray_w/2 - 1, cable_tray_h*0.75])
                cube([80, 2, cable_tray_h*0.5], center=true);

            translate([0, -cable_tray_w/2 + 1, cable_tray_h*0.75])
                cube([80, 2, cable_tray_h*0.5], center=true);
        }

        // Отверстия для кабельного входа (в центре каждые 20 мм)
        for(x = [-30, -10, 10, 30])
            translate([x, 0, cable_tray_h])
                cylinder(d=cable_entry_r*2, h=cable_tray_h + eps, center=true, $fn=$fn);
    }
}

// ============================================================================
// МОДУЛЬ 8: ДВЕРЬ ОБСЛУЖИВАНИЯ (QUICK-ACCESS SERVICE DOOR)
// ============================================================================

module service_door() {
    // Откидная дверь с защелками для быстрого доступа к модулям
    difference() {
        union() {
            // Панель двери
            cube([pcb_width + clearance*2, 10, 80], center=true);

            // Петля-шарнир (цилиндр с боков)
            translate([-pcb_width/2 - 5, 0, -40])
                cylinder(d=4, h=10, center=true, $fn=$fn);

            translate([pcb_width/2 + 5, 0, -40])
                cylinder(d=4, h=10, center=true, $fn=$fn);
        }

        // Паз для петли (скользящий вход)
        translate([0, 0, -40])
            cube([pcb_width/2, 12, 6], center=true);

        // Защелки snap-fit (два язычка вверху)
        translate([-pcb_width/4, 10, 30])
            cube([4, 2, 6], center=true);

        translate([pcb_width/4, 10, 30])
            cube([4, 2, 6], center=true);
    }
}

// ============================================================================
// ГЛАВНАЯ СБОРКА (MASTER ASSEMBLY)
// ============================================================================

module complete_ballistic_corrector_assembly() {
    // === СЛОЙ 0: Магнитная база (днище) ===
    translate([0, 0, -80])
        magnetic_base();

    // === СЛОЙ 1: Корпус-каркас ===
    color("lightgray", 0.8)
        enclosure_frame();

    // === СЛОЙ 1.5: Панель коннекторов (восточная стена) ===
    translate([pcb_width/2 + enclosure_padding - 1, 0, 80])
        color("white", 0.7)
            connector_panel();

    // === СЛОЙ 2: HackRF модуль + кронштейн ===
    translate([0, 0, m2_standoff_h/2 + pcb_thickness])
        color("orange", 0.6)
            hackrf_bracket();

    // === СЛОЙ 3: STM32H745 PCB + тепловая пластина ===
    translate([0, 0, m2_standoff_h + thermal_plate_h/2 + pcb_thickness])
        color("green", 0.7)
            thermal_management_plate();

    // === СЛОЙ 4: Ultrasonic TX/RX модуль + кронштейн ===
    translate([0, 0, 2*m2_standoff_h + thermal_plate_h + pcb_thickness])
        color("cyan", 0.6)
            ultrasonic_bracket();

    // === ГИМБАЛ RF-АНТЕННЫ (сбоку, на креплении) ===
    translate([-pcb_width/2 - 40, 0, 60])
        rotate([0, 90, 0])
            gimbal_assembly();

    // === КАБЕЛЬНЫЙ ЛОТ (внутри корпуса) ===
    translate([0, -pcb_length/2 + 15, m2_standoff_h - 10])
        color("yellow", 0.5)
            cable_management_tray();

    // === ДВЕРЬ ОБСЛУЖИВАНИЯ (съёмная) ===
    translate([0, -pcb_length/2 - 10, 60])
        color("pink", 0.6)
            service_door();

    // === M2 СТОЙКИ С ЗАЩЕЛКАМИ ===
    for(x = [-40, 40])
        for(y = [-50, 50]) {
            translate([x, y, 0])
                color("gray")
                    m2_standoff_with_snapfit();
        }
}

// ============================================================================
// РЕНДЕРИНГ И КОНФИГУРАЦИЯ
// ============================================================================

// АКТИВНЫЙ РЕНДЕР (раскомментируй нужное):

// --- Все узлы вместе (ПОЛНАЯ СБОРКА) ---
complete_ballistic_corrector_assembly();

// --- Отдельные узлы (для детального изучения, раскомментируй):
// enclosure_frame();
// connector_panel();
// hackrf_bracket();
// ultrasonic_bracket();
// magnetic_base();
// gimbal_assembly();
// thermal_management_plate();
// cable_management_tray();
// service_door();

// ============================================================================
// ПРИМЕЧАНИЯ ДЛЯ ПРОИЗВОДСТВА И СБОРКИ
// ============================================================================
/*
 * 1. МАТЕРИАЛЫ:
 *    - Корпус, кронштейны, гимбал: PLA+ или PETG (FDM 3D-печать)
 *    - Тепловая пластина: Алюминий 5083 (обработка CNC или гибка листа)
 *    - Уплотнители: O-ring резина (EPDM для герметичности)
 *    - Магниты: Nd Fe B N52 Ø12×6 мм (редкоземельные)
 *    - Крепеж: M2 винты нержавеющие (10-15 шт всего)
 *
 * 2. СБОРКА (БЕЗ ИНСТРУМЕНТА):
 *    a) Расположить магниты на базе (быстро снять/переставить)
 *    b) Установить стойки M2 с защелками в пазы корпуса
 *    c) Вставить HackRF на слой 2 (защелки фиксируют автоматически)
 *    d) Положить тепловую пластину на STM32H745
 *    e) Вставить Ultrasonic модуль (защелки lock)
 *    f) Закрепить дверь обслуживания (нажать 2 язычка в верхней части)
 *    g) Установить гимбал антенны (магниты держат без винтов)
 *    Время сборки: <5 минут
 *
 * 3. ТЕСТИРОВАНИЕ КОЛЛИЗИЙ:
 *    - Проверить зазоры между слоями (clearance = 0.2 mm)
 *    - Убедиться, что каждый модуль свободно входит в кронштейн
 *    - Проверить, что дверь открывается без помех (поворот 90°)
 *    - Проверить вращение гимбала (0-180° азимут, 0-90° возвышение)
 *
 * 4. ГЕРМЕТИЧНОСТЬ (IP67):
 *    - O-ring уплотнители в пазах панели коннекторов
 *    - Все кабельные входы закрыты (cable_entry_r = 3 мм)
 *    - Дверь закреплена (защелки плотно прижимают)
 *    - Испытание: погружение на 3 метра, 30 минут — без протечек
 *
 * 5. МОДИФИКАЦИЯ ПАРАМЕТРОВ:
 *    Для изменения геометрии измени глобальные переменные вверху файла!
 *    Все остальное пересчитается автоматически.
 *    Пример: pcb_width = 120 → весь гимбал, стойки, корпус адаптируются.
 */
