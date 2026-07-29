"""
Laser Controller Module

Manages laser output power, galvo scanner positioning, and laser-bubble synchronization.

Reference:
- HARDWARE_BOM.md (CNI MGL-III-532 8W laser, Cambridge Tech 6215H galvo scanner)
- VOLUMETRIC_DISPLAY_SPECIFICATION.md (532 nm green, 100×100×100 voxel grid, 10-60 FPS)
"""

import logging
from dataclasses import dataclass
from enum import Enum
from typing import Optional, Dict, Any, Tuple
import math

logger = logging.getLogger(__name__)


class LaserMode(Enum):
    """Laser operation modes."""
    OFF = "off"
    STANDBY = "standby"
    CONTINUOUS = "continuous"
    PULSED = "pulsed"
    SCANNING = "scanning"


@dataclass
class ScanPoint:
    """A single laser scan point (x, y galvo angle)."""
    x_angle_deg: float  # ±20° (Cambridge Tech 6215H)
    y_angle_deg: float  # ±20°
    power_fraction: float  # 0–1.0 relative power at this point


class LaserController:
    """
    Manages DPSS laser and galvo scanner for volumetric rendering.

    Phase 4-5 implementation (detailed design → prototype).
    """

    # Hardware specs (from HARDWARE_BOM.md)
    LASER_POWER_MAX_W = 8.0  # CNI MGL-III-532
    LASER_WAVELENGTH_NM = 532  # Green DPSS
    GALVO_MAX_ANGLE_DEG = 20.0  # ±20° optical (Cambridge Tech 6215H)
    GALVO_STEP_RESPONSE_US = 500  # microseconds

    def __init__(self, wavelength_nm: int = 532, power_w: float = 8.0, use_mock_hw: bool = True):
        """
        Initialize laser controller.

        Args:
            wavelength_nm: Laser wavelength in nanometers (532 for green).
            power_w: Maximum laser power in watts.
            use_mock_hw: Use mock hardware (no serial/USB to actual hardware).
        """
        self.wavelength_nm = wavelength_nm
        self.power_w_max = power_w
        self.use_mock_hw = use_mock_hw

        self._power_current_w = 0.0
        self._mode = LaserMode.OFF
        self._scan_active = False
        self._current_position: Tuple[float, float] = (0.0, 0.0)  # (x_angle, y_angle)

        logger.info(f"LaserController initialized: {wavelength_nm} nm, {power_w} W max")

    def set_power(self, power_w: float):
        """
        Set laser output power.

        Args:
            power_w: Power in watts (0 ≤ power_w ≤ max).

        Raises:
            ValueError: If power is out of range.
        """
        if not (0 <= power_w <= self.power_w_max):
            raise ValueError(f"Power out of range [0, {self.power_w_max}], got {power_w}")

        self._power_current_w = power_w
        logger.info(f"Laser power set to {power_w:.2f} W")

    def start_continuous(self):
        """Start laser in continuous mode."""
        self._mode = LaserMode.CONTINUOUS
        logger.info("Laser started (continuous mode)")

    def start_pulsed(self, frequency_hz: int, duty_cycle: float = 0.5):
        """
        Start laser in pulsed mode.

        Args:
            frequency_hz: Pulse frequency (Hz).
            duty_cycle: Pulse duty cycle (0–1.0).
        """
        if not (1 <= frequency_hz <= 1e6):
            raise ValueError(f"Frequency out of range, got {frequency_hz} Hz")

        self._mode = LaserMode.PULSED
        logger.info(f"Laser started (pulsed mode: {frequency_hz} Hz, duty={duty_cycle:.2f})")

    def start_scan(self):
        """Start laser scanning mode (synchronized with bubble generator)."""
        self._mode = LaserMode.SCANNING
        self._scan_active = True
        logger.info("Laser scanning started")

    def stop_scan(self):
        """Stop laser scanning."""
        self._scan_active = False
        logger.info("Laser scanning stopped")

    def shutdown(self):
        """Shutdown laser (safe power-down)."""
        self._power_current_w = 0.0
        self._mode = LaserMode.OFF
        self._scan_active = False
        logger.info("Laser shutdown")

    def move_galvo(self, x_angle_deg: float, y_angle_deg: float):
        """
        Move galvo scanner to position.

        Args:
            x_angle_deg: X scanner angle (degrees, -20 to +20).
            y_angle_deg: Y scanner angle (degrees, -20 to +20).

        Raises:
            ValueError: If angles are out of range.
        """
        if not (-self.GALVO_MAX_ANGLE_DEG <= x_angle_deg <= self.GALVO_MAX_ANGLE_DEG):
            raise ValueError(f"X angle out of range, got {x_angle_deg}°")
        if not (-self.GALVO_MAX_ANGLE_DEG <= y_angle_deg <= self.GALVO_MAX_ANGLE_DEG):
            raise ValueError(f"Y angle out of range, got {y_angle_deg}°")

        self._current_position = (x_angle_deg, y_angle_deg)
        logger.debug(f"Galvo moved to ({x_angle_deg:.2f}°, {y_angle_deg:.2f}°)")

    def scan_voxel_grid(self, voxel_grid: list, frame_rate_fps: int = 30):
        """
        Scan a voxel grid in volumetric space.

        This is a simplified interface: in reality, you'd compute a laser
        path through the 3D grid, accounting for galvo settle time,
        laser pulse timing, and bubble position.

        Args:
            voxel_grid: 3D numpy array of voxel intensities (0–1).
            frame_rate_fps: Target frame rate.

        Phase 6-7 integration: would interface with real galvo + laser hardware.
        """
        if not self._scan_active:
            logger.warning("Scan requested but scanning not active")
            return

        # Placeholder: in real implementation, this would:
        # 1. Convert 3D voxel grid to 2D laser scan path
        # 2. Account for galvo response time (500 µs per Cambridge spec)
        # 3. Synchronize laser pulses with bubble position/lifetime
        # 4. Write to hardware interface (USB/serial)

        logger.debug(f"Voxel grid scan (frame_rate={frame_rate_fps} FPS)")

    def get_status(self) -> Dict[str, Any]:
        """Get current laser status."""
        return {
            "mode": self._mode.value,
            "power_w": round(self._power_current_w, 2),
            "power_max_w": self.power_w_max,
            "wavelength_nm": self.wavelength_nm,
            "scanning": self._scan_active,
            "galvo_position_deg": {
                "x": round(self._current_position[0], 2),
                "y": round(self._current_position[1], 2),
            },
        }

    def get_telemetry(self) -> Dict[str, Any]:
        """Get real-time telemetry for WebSocket streaming."""
        return {
            "mode": self._mode.value,
            "power_w": round(self._power_current_w, 2),
            "scanning": self._scan_active,
        }
