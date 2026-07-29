"""
Head-Up Display (HUD) Renderer Module

Renders telemetry data as 2D graphics for projection onto windshield.
Supports both Land mode (Volga 2410) and Marine mode (underwater).

Reference:
- docs/HUD_HEAD_UP_DISPLAY.md
- Phase 4-5: Detailed design → Prototype
"""

import logging
import math
from dataclasses import dataclass
from enum import Enum
from typing import Dict, Any, Optional, Tuple, List
from datetime import datetime

logger = logging.getLogger(__name__)


class HUDMode(Enum):
    """Operating modes for HUD."""
    LAND = "land"          # Volga 2410 mode (automotive)
    MARINE = "marine"      # Underwater mode (sonar/depth data)
    DEBUG = "debug"        # Simulator/debug mode
    OFF = "off"


@dataclass
class TelemetryFrame:
    """Single frame of telemetry data for HUD rendering."""
    # Land mode (Volga)
    speed_kmh: float = 0.0
    engine_temp_c: float = 20.0
    battery_voltage_v: float = 12.0
    engine_current_a: float = 0.0

    # Marine mode (underwater)
    depth_m: float = 0.0
    water_temp_c: float = 15.0
    pressure_bar: float = 1.0
    salinity_ppt: float = 35.0  # Practical Salinity Units

    # Common
    mode: HUDMode = HUDMode.LAND
    alarms: List[str] = None  # ["knock_detected", "overtemp", "low_voltage"]
    timestamp: datetime = None


class HUDRenderer:
    """
    Renders telemetry data as 2D graphics for HUD projection.

    Phase 4-5 implementation (detailed design → prototype).
    Targets: 1280×800 resolution (DLP TRP-4500 spec).
    """

    # Display specs (from docs/HUD_HEAD_UP_DISPLAY.md)
    DISPLAY_WIDTH = 1280
    DISPLAY_HEIGHT = 800
    DISPLAY_BRIGHTNESS_CD_M2 = 4000

    # Color palette for HUD (RGB, 0-255)
    COLOR_PRIMARY = (0, 255, 100)      # Green (land), cyan-ish for marine
    COLOR_WARNING = (255, 200, 0)      # Amber
    COLOR_ALARM = (255, 50, 50)        # Red
    COLOR_BACKGROUND = (0, 0, 0)       # Black (transparent on combiner)
    COLOR_TEXT = (0, 255, 100)

    # Font sizes (proportional to display height)
    FONT_SIZE_LARGE = int(DISPLAY_HEIGHT * 0.08)
    FONT_SIZE_MEDIUM = int(DISPLAY_HEIGHT * 0.05)
    FONT_SIZE_SMALL = int(DISPLAY_HEIGHT * 0.03)

    def __init__(self, mode: HUDMode = HUDMode.LAND, brightness_percent: float = 100.0):
        """
        Initialize HUD renderer.

        Args:
            mode: Operating mode (LAND, MARINE, DEBUG).
            brightness_percent: Display brightness (0-100).
        """
        self.mode = mode
        self.brightness_percent = brightness_percent

        # Frame buffer (simulated 2D pixel array)
        # In real implementation, this would be sent to micro-display controller
        self.frame_buffer: List[List[Tuple[int, int, int]]] = [
            [(0, 0, 0) for _ in range(self.DISPLAY_WIDTH)]
            for _ in range(self.DISPLAY_HEIGHT)
        ]

        self._active = False
        self._frame_count = 0

        logger.info(f"HUDRenderer initialized (mode={mode.value}, brightness={brightness_percent}%)")

    def render_frame(self, telemetry: TelemetryFrame) -> Dict[str, Any]:
        """
        Render a single HUD frame from telemetry data.

        Args:
            telemetry: Telemetry data frame.

        Returns:
            Metadata about rendered frame (resolution, colors used, warnings).
        """
        # Clear frame buffer
        self.frame_buffer = [
            [(0, 0, 0) for _ in range(self.DISPLAY_WIDTH)]
            for _ in range(self.DISPLAY_HEIGHT)
        ]

        self._active = True
        self._frame_count += 1

        # Render based on mode
        if self.mode == HUDMode.LAND:
            self._render_land_mode(telemetry)
        elif self.mode == HUDMode.MARINE:
            self._render_marine_mode(telemetry)
        elif self.mode == HUDMode.DEBUG:
            self._render_debug_mode(telemetry)

        # Render alarms on top
        if telemetry.alarms:
            self._render_alarms(telemetry.alarms)

        return {
            "frame": self._frame_count,
            "mode": self.mode.value,
            "resolution": f"{self.DISPLAY_WIDTH}x{self.DISPLAY_HEIGHT}",
            "brightness_cd_m2": int(self.DISPLAY_BRIGHTNESS_CD_M2 * self.brightness_percent / 100),
            "alarms": telemetry.alarms or [],
        }

    def _render_land_mode(self, telemetry: TelemetryFrame):
        """Render HUD for Volga 2410 (automotive mode)."""
        # Central speedometer
        self._draw_speedometer(telemetry.speed_kmh)

        # Engine temperature indicator (top-left)
        self._draw_engine_status(telemetry.engine_temp_c, telemetry.engine_current_a)

        # Battery voltage (top-right)
        self._draw_battery_status(telemetry.battery_voltage_v)

        # Bottom: navigation/gear info (placeholder)
        self._draw_text(
            f"P  ENG OK  {telemetry.engine_temp_c:.1f}°C",
            x=50, y=self.DISPLAY_HEIGHT - 100,
            size=self.FONT_SIZE_MEDIUM,
            color=self.COLOR_TEXT
        )

    def _render_marine_mode(self, telemetry: TelemetryFrame):
        """Render HUD for underwater operation (diving/ROV pilot)."""
        # Depth gauge (center-large)
        self._draw_depth_gauge(telemetry.depth_m)

        # Water temperature (top-left)
        self._draw_text(
            f"TEMP: {telemetry.water_temp_c:.1f}°C",
            x=50, y=50,
            size=self.FONT_SIZE_MEDIUM,
            color=self.COLOR_TEXT
        )

        # Pressure (top-center)
        self._draw_text(
            f"PRESSURE: {telemetry.pressure_bar:.1f} bar",
            x=self.DISPLAY_WIDTH // 2 - 150, y=50,
            size=self.FONT_SIZE_MEDIUM,
            color=self.COLOR_TEXT
        )

        # Salinity (top-right)
        self._draw_text(
            f"SALINITY: {telemetry.salinity_ppt:.1f} ppt",
            x=self.DISPLAY_WIDTH - 350, y=50,
            size=self.FONT_SIZE_MEDIUM,
            color=self.COLOR_TEXT
        )

        # Bottom: dive time, compass (placeholder)
        self._draw_text(
            "DIVE TIME: 15:34  COMPASS: 045°",
            x=50, y=self.DISPLAY_HEIGHT - 100,
            size=self.FONT_SIZE_MEDIUM,
            color=self.COLOR_TEXT
        )

    def _render_debug_mode(self, telemetry: TelemetryFrame):
        """Render debug/simulator display."""
        # Show all telemetry at once
        debug_lines = [
            f"MODE: {self.mode.value.upper()}",
            f"Speed: {telemetry.speed_kmh:.1f} km/h",
            f"Depth: {telemetry.depth_m:.1f} m",
            f"Eng Temp: {telemetry.engine_temp_c:.1f}°C",
            f"Water Temp: {telemetry.water_temp_c:.1f}°C",
            f"Battery: {telemetry.battery_voltage_v:.1f}V",
            f"Pressure: {telemetry.pressure_bar:.1f} bar",
            f"Frame: {self._frame_count}",
        ]

        for i, line in enumerate(debug_lines):
            self._draw_text(
                line,
                x=50, y=50 + i * 50,
                size=self.FONT_SIZE_SMALL,
                color=self.COLOR_TEXT
            )

    def _render_alarms(self, alarms: List[str]):
        """Render alarm indicators (top center)."""
        if not alarms:
            return

        alarm_text = " | ".join(alarms).upper()
        self._draw_text(
            alarm_text,
            x=self.DISPLAY_WIDTH // 2 - 200, y=10,
            size=self.FONT_SIZE_MEDIUM,
            color=self.COLOR_ALARM
        )

    def _draw_speedometer(self, speed_kmh: float):
        """Draw speedometer (land mode)."""
        # Center circle
        center_x = self.DISPLAY_WIDTH // 2
        center_y = self.DISPLAY_HEIGHT // 2

        # Speed value (large text)
        speed_text = f"{int(speed_kmh)} km/h"
        self._draw_text(
            speed_text,
            x=center_x - 150, y=center_y - 50,
            size=self.FONT_SIZE_LARGE,
            color=self.COLOR_PRIMARY
        )

        # Speed bar (visual indicator)
        bar_width = int((speed_kmh / 180.0) * (self.DISPLAY_WIDTH * 0.6))  # Max 180 km/h
        bar_width = min(bar_width, int(self.DISPLAY_WIDTH * 0.6))

        color = self.COLOR_PRIMARY
        if speed_kmh > 130:  # Warning
            color = self.COLOR_WARNING
        if speed_kmh > 160:  # Alarm
            color = self.COLOR_ALARM

        # Draw bar (simplified pixel-based)
        bar_x = int(self.DISPLAY_WIDTH * 0.2)
        bar_y = center_y + 80
        bar_height = 20

        # Fill pixels in range
        for x in range(bar_x, min(bar_x + bar_width, self.DISPLAY_WIDTH)):
            for y in range(bar_y, min(bar_y + bar_height, self.DISPLAY_HEIGHT)):
                if 0 <= y < self.DISPLAY_HEIGHT and 0 <= x < self.DISPLAY_WIDTH:
                    self.frame_buffer[y][x] = color

    def _draw_depth_gauge(self, depth_m: float):
        """Draw depth gauge (marine mode)."""
        center_x = self.DISPLAY_WIDTH // 2
        center_y = self.DISPLAY_HEIGHT // 2

        # Depth value (large text)
        depth_text = f"{depth_m:.1f} m"
        self._draw_text(
            depth_text,
            x=center_x - 150, y=center_y - 100,
            size=self.FONT_SIZE_LARGE,
            color=self.COLOR_PRIMARY
        )

        # Depth bar (0-100m scale)
        bar_height = int((depth_m / 100.0) * (self.DISPLAY_HEIGHT * 0.5))
        bar_height = min(bar_height, int(self.DISPLAY_HEIGHT * 0.5))

        color = self.COLOR_PRIMARY
        if depth_m > 40:  # Deep water
            color = self.COLOR_WARNING
        if depth_m > 80:  # Very deep
            color = self.COLOR_ALARM

        # Draw vertical bar on left
        bar_x = 50
        bar_y = self.DISPLAY_HEIGHT - 100
        bar_width = 30

        for x in range(bar_x, bar_x + bar_width):
            for y in range(max(0, bar_y - bar_height), bar_y):
                if 0 <= y < self.DISPLAY_HEIGHT and 0 <= x < self.DISPLAY_WIDTH:
                    self.frame_buffer[y][x] = color

    def _draw_engine_status(self, temp_c: float, current_a: float):
        """Draw engine status panel (land mode)."""
        status_text = f"ENG: {temp_c:.1f}°C {current_a:.1f}A"
        color = self.COLOR_PRIMARY

        if temp_c > 95:
            color = self.COLOR_WARNING
        if temp_c > 110:
            color = self.COLOR_ALARM

        self._draw_text(
            status_text,
            x=50, y=50,
            size=self.FONT_SIZE_MEDIUM,
            color=color
        )

    def _draw_battery_status(self, voltage_v: float):
        """Draw battery status panel (land mode)."""
        status_text = f"BAT: {voltage_v:.1f}V"
        color = self.COLOR_PRIMARY

        if voltage_v < 11.5:
            color = self.COLOR_WARNING
        if voltage_v < 11.0:
            color = self.COLOR_ALARM

        self._draw_text(
            status_text,
            x=self.DISPLAY_WIDTH - 300, y=50,
            size=self.FONT_SIZE_MEDIUM,
            color=color
        )

    def _draw_text(self, text: str, x: int, y: int, size: int, color: Tuple[int, int, int]):
        """
        Draw text on frame buffer (simplified placeholder).

        Real implementation would use PIL/Pillow or hardware font rendering.
        This just marks the region where text would appear.
        """
        # Placeholder: mark a rectangular region with color
        # Actual rendering happens on micro-display hardware
        char_width = size // 2
        text_width = len(text) * char_width

        for px in range(max(0, x), min(self.DISPLAY_WIDTH, x + text_width)):
            for py in range(max(0, y), min(self.DISPLAY_HEIGHT, y + size)):
                if 0 <= py < self.DISPLAY_HEIGHT and 0 <= px < self.DISPLAY_WIDTH:
                    self.frame_buffer[py][px] = color

    def get_status(self) -> Dict[str, Any]:
        """Get current HUD status."""
        return {
            "mode": self.mode.value,
            "active": self._active,
            "brightness_percent": self.brightness_percent,
            "brightness_cd_m2": int(self.DISPLAY_BRIGHTNESS_CD_M2 * self.brightness_percent / 100),
            "frame_count": self._frame_count,
            "resolution": f"{self.DISPLAY_WIDTH}x{self.DISPLAY_HEIGHT}",
        }

    def set_brightness(self, percent: float):
        """Set display brightness (0-100%)."""
        if not (0 <= percent <= 100):
            raise ValueError(f"Brightness out of range, got {percent}")
        self.brightness_percent = percent
        logger.info(f"HUD brightness set to {percent}%")

    def set_mode(self, mode: HUDMode):
        """Set HUD operating mode."""
        self.mode = mode
        logger.info(f"HUD mode set to {mode.value}")
