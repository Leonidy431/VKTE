"""
Bubble Generator Module

Implements acoustic cavitation bubble generation for volumetric display.

Reference:
- docs/BUBBLE_TECHNOLOGY.md (Acoustic cavitation: stable vs inertial, Langevin transducer)
- HARDWARE_BOM.md (Steminc SMBLTD45F40H 40 kHz transducer)

Physics:
- Cavitation threshold: amplitude-dependent, solute-gas dependent
- Resonant frequency: Minnaert formula f_0 = (1/2π) * sqrt(3*γ*P_0 / ρ*R_0²)
  where P_0=atmospheric pressure, R_0=bubble radius, ρ=liquid density, γ=heat capacity ratio
- Bubble lifetime (stable cavitation): 10-100ms (milliseconds per acoustic cycle)
"""

import logging
import asyncio
from dataclasses import dataclass
from enum import Enum
from typing import Optional, Dict, Any
import math

logger = logging.getLogger(__name__)


class BubbleStabilityRegime(Enum):
    """Cavitation regimes per BUBBLE_TECHNOLOGY.md"""
    STABLE = "stable"  # Oscillating bubbles, persist for multiple frames
    INERTIAL = "inertial"  # Violently collapsing bubbles, erosive, short-lived
    OFF = "off"  # No cavitation


@dataclass
class BubbleMetrics:
    """Real-time bubble metrics."""
    frequency_hz: int
    duty_cycle: float
    resonant_freq_hz: float
    bubble_radius_um: float
    density_percent: float  # 0-100, estimated bubble density
    stability_regime: BubbleStabilityRegime
    transducer_temp_celsius: Optional[float] = None


class BubbleGenerator:
    """
    Manages acoustic cavitation bubble generation via Langevin transducers.

    Phase 4-5 implementation (detailed design → prototype).
    """

    # Minnaert resonant frequency calculation constants
    ATMOSPHERIC_PRESSURE_PA = 101325.0
    WATER_DENSITY_KG_M3 = 1025.0  # Seawater (from VOLUMETRIC_DISPLAY_SPECIFICATION.md)
    HEAT_CAPACITY_RATIO = 1.4  # Air
    CAVITATION_THRESHOLD_PA = 1e5  # ~1 atm acoustic pressure (empirical)

    def __init__(
        self,
        frequency_hz: int = 40000,
        duty_cycle: float = 0.5,
        stabilizer_type: str = "saponin",
        use_mock_hw: bool = True,
    ):
        """
        Initialize bubble generator.

        Args:
            frequency_hz: Transducer driving frequency (40 kHz for Steminc board).
            duty_cycle: Fraction of time actively driving (0–1.0).
            stabilizer_type: Surfactant for bubble stabilization ("saponin", "SDS", "phospholipid").
            use_mock_hw: Use mock hardware (no serial/USB connection).
        """
        self.frequency_hz = frequency_hz
        self.duty_cycle = duty_cycle
        self.stabilizer_type = stabilizer_type
        self.use_mock_hw = use_mock_hw

        self._running = False
        self._acoustic_amplitude_pa = 0.0  # Acoustic pressure amplitude
        self._bubble_radius_um = 50.0  # Resonant bubble size (micrometers)
        self._density_percent = 0.0

        logger.info(
            f"BubbleGenerator initialized: {frequency_hz} Hz, "
            f"duty_cycle={duty_cycle:.2f}, stabilizer={stabilizer_type}"
        )

    def start(self):
        """Start bubble generation."""
        if self._running:
            logger.warning("Bubble generation already running")
            return

        self._running = True
        logger.info("Bubble generation started")

    def stop(self):
        """Stop bubble generation."""
        self._running = False
        self._acoustic_amplitude_pa = 0.0
        logger.info("Bubble generation stopped")

    def set_frequency(self, frequency_hz: int):
        """
        Set transducer driving frequency.

        Args:
            frequency_hz: Frequency in Hz (typical: 20-80 kHz).

        Raises:
            ValueError: If frequency is out of acceptable range.
        """
        if not (20000 <= frequency_hz <= 80000):
            raise ValueError(f"Frequency must be 20-80 kHz, got {frequency_hz} Hz")

        self.frequency_hz = frequency_hz
        logger.info(f"Bubble frequency set to {frequency_hz} Hz")

    def set_duty_cycle(self, duty_cycle: float):
        """
        Set transducer duty cycle (0–1.0).

        Duty cycle is the primary throttle for bubble *density*.
        Higher duty cycle = higher density = more voxels addressable but higher power.

        Args:
            duty_cycle: Fraction of time actively driving (0–1.0).

        Raises:
            ValueError: If duty_cycle is out of range.
        """
        if not (0.0 <= duty_cycle <= 1.0):
            raise ValueError(f"Duty cycle must be 0–1.0, got {duty_cycle}")

        self.duty_cycle = duty_cycle
        logger.info(f"Bubble duty cycle set to {duty_cycle:.2f}")

    def set_acoustic_pressure(self, pressure_pa: float):
        """
        Set acoustic pressure amplitude (driving force).

        Args:
            pressure_pa: Acoustic pressure in pascals. Typical range: 50 kPa–500 kPa.

        Raises:
            ValueError: If pressure is out of range.
        """
        if not (0 <= pressure_pa <= 500e3):
            raise ValueError(f"Acoustic pressure out of range, got {pressure_pa} Pa")

        self._acoustic_amplitude_pa = pressure_pa

        # Update stability regime based on threshold
        if pressure_pa < self.CAVITATION_THRESHOLD_PA:
            self._stability_regime = BubbleStabilityRegime.OFF
        elif pressure_pa < 3 * self.CAVITATION_THRESHOLD_PA:
            self._stability_regime = BubbleStabilityRegime.STABLE
        else:
            self._stability_regime = BubbleStabilityRegime.INERTIAL

        logger.debug(f"Acoustic pressure set to {pressure_pa:.1e} Pa ({self._stability_regime.value})")

    def set_bubble_radius(self, radius_um: float):
        """
        Set target bubble resonant radius.

        Args:
            radius_um: Bubble radius in micrometers (typical: 10–100 µm).

        Raises:
            ValueError: If radius is out of typical range.
        """
        if not (1 <= radius_um <= 500):
            raise ValueError(f"Bubble radius out of range, got {radius_um} µm")

        self._bubble_radius_um = radius_um
        logger.debug(f"Target bubble radius set to {radius_um:.1f} µm")

    def _calculate_resonant_frequency(self) -> float:
        """
        Calculate Minnaert resonant frequency for current bubble radius.

        Formula: f_0 = (1/2π) * sqrt(3*γ*P_0 / (ρ*R_0²))

        Returns:
            Resonant frequency in Hz.
        """
        radius_m = self._bubble_radius_um * 1e-6
        numerator = 3.0 * self.HEAT_CAPACITY_RATIO * self.ATMOSPHERIC_PRESSURE_PA
        denominator = self.WATER_DENSITY_KG_M3 * (radius_m ** 2)
        f0_hz = (1.0 / (2.0 * math.pi)) * math.sqrt(numerator / denominator)
        return f0_hz

    def _update_density(self):
        """
        Estimate bubble density based on duty cycle and stability regime.

        This is a simplification; real density depends on transducer design,
        surfactant concentration, and thermal management.
        """
        if not self._running or self._stability_regime == BubbleStabilityRegime.OFF:
            self._density_percent = 0.0
        elif self._stability_regime == BubbleStabilityRegime.STABLE:
            # Stable cavitation: density proportional to duty cycle
            self._density_percent = min(100.0, 80.0 * self.duty_cycle)
        else:
            # Inertial cavitation: lower effective density, more sporadic
            self._density_percent = min(100.0, 40.0 * self.duty_cycle)

    def get_status(self) -> Dict[str, Any]:
        """Get current status."""
        self._update_density()
        resonant_freq = self._calculate_resonant_frequency()

        return {
            "running": self._running,
            "frequency_hz": self.frequency_hz,
            "duty_cycle": self.duty_cycle,
            "bubble_radius_um": self._bubble_radius_um,
            "resonant_frequency_hz": int(resonant_freq),
            "acoustic_pressure_pa": self._acoustic_amplitude_pa,
            "stability_regime": self._stability_regime.value,
            "density_percent": round(self._density_percent, 1),
            "stabilizer": self.stabilizer_type,
        }

    def get_telemetry(self) -> Dict[str, Any]:
        """Get real-time telemetry for WebSocket streaming."""
        return {
            "running": self._running,
            "density_percent": round(self._density_percent, 1),
            "acoustic_pressure_pa": self._acoustic_amplitude_pa,
            "stability_regime": self._stability_regime.value,
        }


# Default stability regime
BubbleGenerator._stability_regime = BubbleStabilityRegime.OFF
