"""Unit tests for RenderingCoordinator."""

import pytest
from unittest.mock import patch
from app.coordinator import RenderingCoordinator


def make_coordinator(**overrides):
    defaults = dict(
        bubble_frequency=40000,
        bubble_duty_cycle=0.5,
        stabilizer_agent="saponin",
        laser_wavelength=532,
        laser_power=8.0,
        voxel_resolution=20,  # small grid keeps tests fast
        frame_rate=30,
        mock_hardware=True,
    )
    defaults.update(overrides)
    return RenderingCoordinator(**defaults)


class TestRenderingCoordinatorInit:
    """Test full (non-degraded) initialization."""

    def test_initialize_all_subsystems(self):
        coord = make_coordinator()
        coord.initialize()

        assert coord.bubble_gen is not None
        assert coord.laser_ctrl is not None
        assert coord.renderer is not None
        assert coord.hud_renderer is not None
        assert coord.is_degraded is False
        assert coord.degraded_subsystems == {}

    def test_initialize_passes_through_params(self):
        coord = make_coordinator(bubble_frequency=45000, laser_wavelength=1064)
        coord.initialize()

        assert coord.bubble_gen.frequency_hz == 45000
        assert coord.laser_ctrl.wavelength_nm == 1064


class TestRenderingCoordinatorDegradedMode:
    """Test graceful per-subsystem degradation."""

    def test_bubble_generator_failure_is_isolated(self):
        coord = make_coordinator()
        with patch(
            "app.coordinator.BubbleGenerator", side_effect=RuntimeError("no hardware")
        ):
            coord.initialize()

        assert coord.bubble_gen is None
        assert coord.laser_ctrl is not None
        assert coord.renderer is not None
        assert coord.hud_renderer is not None
        assert coord.is_degraded is True
        assert "bubble_generator" in coord.degraded_subsystems
        assert "no hardware" in coord.degraded_subsystems["bubble_generator"]

    def test_laser_controller_failure_is_isolated(self):
        coord = make_coordinator()
        with patch(
            "app.coordinator.LaserController", side_effect=RuntimeError("no galvo")
        ):
            coord.initialize()

        assert coord.laser_ctrl is None
        assert coord.bubble_gen is not None
        assert "laser_controller" in coord.degraded_subsystems

    def test_all_subsystems_failing(self):
        coord = make_coordinator()
        with patch(
            "app.coordinator.BubbleGenerator", side_effect=RuntimeError("e1")
        ), patch(
            "app.coordinator.LaserController", side_effect=RuntimeError("e2")
        ), patch(
            "app.coordinator.VolumetricRenderer", side_effect=RuntimeError("e3")
        ), patch(
            "app.coordinator.HUDRenderer", side_effect=RuntimeError("e4")
        ):
            coord.initialize()

        assert coord.bubble_gen is None
        assert coord.laser_ctrl is None
        assert coord.renderer is None
        assert coord.hud_renderer is None
        assert set(coord.degraded_subsystems.keys()) == {
            "bubble_generator",
            "laser_controller",
            "volumetric_renderer",
            "hud_renderer",
        }

    def test_reinitialize_clears_previous_degraded_state(self):
        coord = make_coordinator()
        with patch(
            "app.coordinator.BubbleGenerator", side_effect=RuntimeError("no hardware")
        ):
            coord.initialize()
        assert coord.is_degraded is True

        coord.initialize()  # second call, no patch active this time
        assert coord.is_degraded is False
        assert coord.bubble_gen is not None


class TestRenderingCoordinatorShutdown:
    """Test shutdown behavior."""

    def test_shutdown_stops_subsystems(self):
        coord = make_coordinator()
        coord.initialize()
        coord.bubble_gen.start()
        assert coord.bubble_gen._running is True

        coord.shutdown()
        assert coord.bubble_gen._running is False

    def test_shutdown_with_nothing_initialized_is_noop(self):
        coord = make_coordinator()
        # Never called initialize(); all subsystems are None.
        coord.shutdown()  # must not raise

    def test_shutdown_bubble_stop_error_is_logged_not_raised(self):
        coord = make_coordinator()
        coord.initialize()
        with patch.object(
            coord.bubble_gen, "stop", side_effect=RuntimeError("stop failed")
        ):
            coord.shutdown()  # must not raise

    def test_shutdown_laser_shutdown_error_is_logged_not_raised(self):
        coord = make_coordinator()
        coord.initialize()
        with patch.object(
            coord.laser_ctrl, "shutdown", side_effect=RuntimeError("shutdown failed")
        ):
            coord.shutdown()  # must not raise


class TestRenderingCoordinatorLaserScanSync:
    """Test the bubble/laser synchronization guard."""

    def test_start_laser_scan_without_laser_controller(self):
        coord = make_coordinator()
        with patch(
            "app.coordinator.LaserController", side_effect=RuntimeError("no galvo")
        ):
            coord.initialize()

        with pytest.raises(RuntimeError, match="Laser controller not initialized"):
            coord.start_laser_scan()

    def test_start_laser_scan_without_bubbles_running(self):
        coord = make_coordinator()
        coord.initialize()

        with pytest.raises(RuntimeError, match="bubble generation is not active"):
            coord.start_laser_scan()

    def test_start_laser_scan_without_bubble_generator(self):
        coord = make_coordinator()
        with patch(
            "app.coordinator.BubbleGenerator", side_effect=RuntimeError("no hardware")
        ):
            coord.initialize()

        with pytest.raises(RuntimeError, match="bubble generation is not active"):
            coord.start_laser_scan()

    def test_start_laser_scan_with_bubbles_running(self):
        coord = make_coordinator()
        coord.initialize()
        coord.bubble_gen.start()

        coord.start_laser_scan()
        assert coord.laser_ctrl._scan_active is True


class TestRenderingCoordinatorStatus:
    """Test aggregated status reporting."""

    def test_get_status_fully_initialized(self):
        coord = make_coordinator()
        coord.initialize()
        status = coord.get_status()

        assert status["degraded"] is False
        assert status["degraded_subsystems"] == {}
        assert status["bubble_generator"] is not None
        assert status["laser_controller"] is not None
        assert status["renderer"] is not None
        assert status["hud"] is not None

    def test_get_status_degraded(self):
        coord = make_coordinator()
        with patch(
            "app.coordinator.BubbleGenerator", side_effect=RuntimeError("no hardware")
        ):
            coord.initialize()
        status = coord.get_status()

        assert status["degraded"] is True
        assert "bubble_generator" in status["degraded_subsystems"]
        assert status["bubble_generator"] is None
        assert status["laser_controller"] is not None

    def test_get_status_nothing_initialized(self):
        coord = make_coordinator()
        status = coord.get_status()

        assert status["bubble_generator"] is None
        assert status["laser_controller"] is None
        assert status["renderer"] is None
        assert status["hud"] is None
