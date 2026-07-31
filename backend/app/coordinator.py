"""
Rendering Coordinator

Owns the four volumetric-display subsystems (bubble generator, laser
controller, renderer, HUD renderer) and initializes them independently so a
single hardware failure degrades that one subsystem instead of crashing the
whole application (Sprint 2, item A3: graceful degradation).

Also enforces the basic cross-subsystem invariant that laser scanning should
not run without bubble generation active (Sprint 2, item A2: synchronization).

Reference: ARCHITECTURE_AUDIT.md, Sprint 2 (Synchronization & Degradation).
"""

import logging
from typing import Any, Callable, Dict, Optional

from app.modules.volumetric.bubble_generator import BubbleGenerator
from app.modules.volumetric.laser_controller import LaserController
from app.modules.volumetric.volumetric_renderer import VolumetricRenderer
from app.modules.volumetric.hud_renderer import HUDRenderer, HUDMode

logger = logging.getLogger(__name__)


class RenderingCoordinator:
    """
    Initializes and synchronizes the volumetric display subsystems.

    Each subsystem is brought up independently: if one fails to initialize
    (e.g. hardware not present), that subsystem is left as `None` and marked
    degraded rather than aborting startup of the other three. Callers should
    check `is_degraded` / `degraded_subsystems` and treat a `None` subsystem
    the same way the API layer already treats "not initialized" (503).
    """

    def __init__(
        self,
        bubble_frequency: int,
        bubble_duty_cycle: float,
        stabilizer_agent: str,
        laser_wavelength: int,
        laser_power: float,
        voxel_resolution: int,
        frame_rate: int,
        mock_hardware: bool,
    ):
        self._bubble_frequency = bubble_frequency
        self._bubble_duty_cycle = bubble_duty_cycle
        self._stabilizer_agent = stabilizer_agent
        self._laser_wavelength = laser_wavelength
        self._laser_power = laser_power
        self._voxel_resolution = voxel_resolution
        self._frame_rate = frame_rate
        self._mock_hardware = mock_hardware

        self.bubble_gen: Optional[BubbleGenerator] = None
        self.laser_ctrl: Optional[LaserController] = None
        self.renderer: Optional[VolumetricRenderer] = None
        self.hud_renderer: Optional[HUDRenderer] = None

        self.degraded_subsystems: Dict[str, str] = {}  # name -> error message

    def _init_subsystem(self, name: str, factory: Callable[[], Any]) -> Optional[Any]:
        """Initialize one subsystem, isolating its failure from the others."""
        try:
            instance = factory()
            logger.info(f"{name} initialized successfully")
            return instance
        except Exception as e:
            logger.error(
                f"{name} failed to initialize (degraded mode): {e}", exc_info=True
            )
            self.degraded_subsystems[name] = str(e)
            return None

    def initialize(self):
        """Bring up all subsystems, tolerating per-subsystem failures."""
        self.degraded_subsystems = {}

        self.bubble_gen = self._init_subsystem(
            "bubble_generator",
            lambda: BubbleGenerator(
                frequency_hz=self._bubble_frequency,
                duty_cycle=self._bubble_duty_cycle,
                stabilizer_type=self._stabilizer_agent,
                use_mock_hw=self._mock_hardware,
            ),
        )

        self.laser_ctrl = self._init_subsystem(
            "laser_controller",
            lambda: LaserController(
                wavelength_nm=self._laser_wavelength,
                power_w=self._laser_power,
                use_mock_hw=self._mock_hardware,
            ),
        )

        self.renderer = self._init_subsystem(
            "volumetric_renderer",
            lambda: VolumetricRenderer(
                grid_resolution=self._voxel_resolution,
                frame_rate_fps=self._frame_rate,
            ),
        )

        self.hud_renderer = self._init_subsystem(
            "hud_renderer",
            lambda: HUDRenderer(mode=HUDMode.LAND, brightness_percent=100.0),
        )

        if self.degraded_subsystems:
            logger.warning(
                f"Started in degraded mode; unavailable subsystems: "
                f"{list(self.degraded_subsystems.keys())}"
            )

    def shutdown(self):
        """Shut down whichever subsystems came up, tolerating per-subsystem failures."""
        if self.bubble_gen:
            try:
                self.bubble_gen.stop()
            except Exception as e:
                logger.error(f"Error stopping bubble_generator: {e}", exc_info=True)

        if self.laser_ctrl:
            try:
                self.laser_ctrl.shutdown()
            except Exception as e:
                logger.error(
                    f"Error shutting down laser_controller: {e}", exc_info=True
                )

    @property
    def is_degraded(self) -> bool:
        return bool(self.degraded_subsystems)

    def start_laser_scan(self):
        """Start laser scanning, refusing to do so without bubble generation active.

        Scanning the laser through a volume with no cavitation bubbles present
        has no visible effect and just burns laser dwell time/power, so this
        enforces the ordering the two hardware modules rely on implicitly.
        """
        if not self.laser_ctrl:
            raise RuntimeError("Laser controller not initialized")
        if not self.bubble_gen or not self.bubble_gen._running:
            raise RuntimeError(
                "Cannot start laser scan: bubble generation is not active"
            )
        self.laser_ctrl.start_scan()

    def get_status(self) -> Dict[str, Any]:
        """Aggregate status across all subsystems, including degraded ones."""
        return {
            "degraded": self.is_degraded,
            "degraded_subsystems": dict(self.degraded_subsystems),
            "bubble_generator": self.bubble_gen.get_status()
            if self.bubble_gen
            else None,
            "laser_controller": self.laser_ctrl.get_status()
            if self.laser_ctrl
            else None,
            "renderer": self.renderer.get_status() if self.renderer else None,
            "hud": self.hud_renderer.get_status() if self.hud_renderer else None,
        }
