"""Unit tests for LaserController module."""

import pytest
from app.modules.volumetric.laser_controller import (
    LaserController,
    LaserMode,
    ScanPoint,
)


class TestLaserControllerInit:
    """Test LaserController initialization."""

    def test_init_defaults(self):
        """Test default initialization."""
        ctrl = LaserController()
        assert ctrl.wavelength_nm == 532
        assert ctrl.power_w_max == 8.0
        assert ctrl.use_mock_hw is True
        assert ctrl._power_current_w == 0.0
        assert ctrl._mode == LaserMode.OFF
        assert ctrl._scan_active is False

    def test_init_custom_params(self):
        """Test initialization with custom parameters."""
        ctrl = LaserController(wavelength_nm=1064, power_w=10.0, use_mock_hw=False)
        assert ctrl.wavelength_nm == 1064
        assert ctrl.power_w_max == 10.0
        assert ctrl.use_mock_hw is False


class TestLaserControllerPower:
    """Test laser power control."""

    def test_set_power_valid(self):
        """Test setting valid power."""
        ctrl = LaserController()
        ctrl.set_power(4.0)
        assert ctrl._power_current_w == 4.0

    def test_set_power_zero(self):
        """Test setting power to zero."""
        ctrl = LaserController()
        ctrl.set_power(0.0)
        assert ctrl._power_current_w == 0.0

    def test_set_power_max(self):
        """Test setting power to maximum."""
        ctrl = LaserController()
        ctrl.set_power(8.0)
        assert ctrl._power_current_w == 8.0

    def test_set_power_out_of_range_negative(self):
        """Test setting power below zero."""
        ctrl = LaserController()
        with pytest.raises(ValueError, match="Power out of range"):
            ctrl.set_power(-0.1)

    def test_set_power_out_of_range_high(self):
        """Test setting power above maximum."""
        ctrl = LaserController()
        with pytest.raises(ValueError, match="Power out of range"):
            ctrl.set_power(8.1)


class TestLaserControllerModes:
    """Test laser operating modes."""

    def test_start_continuous(self):
        """Test starting continuous mode."""
        ctrl = LaserController()
        ctrl.start_continuous()
        assert ctrl._mode == LaserMode.CONTINUOUS

    def test_start_pulsed_valid(self):
        """Test starting pulsed mode with valid frequency."""
        ctrl = LaserController()
        ctrl.start_pulsed(frequency_hz=40000, duty_cycle=0.5)
        assert ctrl._mode == LaserMode.PULSED

    def test_start_pulsed_min_frequency(self):
        """Test pulsed mode at minimum frequency."""
        ctrl = LaserController()
        ctrl.start_pulsed(frequency_hz=1)
        assert ctrl._mode == LaserMode.PULSED

    def test_start_pulsed_max_frequency(self):
        """Test pulsed mode at maximum frequency."""
        ctrl = LaserController()
        ctrl.start_pulsed(frequency_hz=int(1e6))
        assert ctrl._mode == LaserMode.PULSED

    def test_start_pulsed_out_of_range_low(self):
        """Test pulsed mode with frequency below minimum."""
        ctrl = LaserController()
        with pytest.raises(ValueError, match="Frequency out of range"):
            ctrl.start_pulsed(frequency_hz=0)

    def test_start_pulsed_out_of_range_high(self):
        """Test pulsed mode with frequency above maximum."""
        ctrl = LaserController()
        with pytest.raises(ValueError, match="Frequency out of range"):
            ctrl.start_pulsed(frequency_hz=int(1e6) + 1)

    def test_start_scan(self):
        """Test starting scan mode."""
        ctrl = LaserController()
        ctrl.start_scan()
        assert ctrl._mode == LaserMode.SCANNING
        assert ctrl._scan_active is True

    def test_stop_scan(self):
        """Test stopping scan mode."""
        ctrl = LaserController()
        ctrl.start_scan()
        ctrl.stop_scan()
        assert ctrl._scan_active is False

    def test_shutdown(self):
        """Test laser shutdown."""
        ctrl = LaserController()
        ctrl.set_power(5.0)
        ctrl.start_continuous()
        ctrl.shutdown()
        assert ctrl._power_current_w == 0.0
        assert ctrl._mode == LaserMode.OFF
        assert ctrl._scan_active is False


class TestLaserControllerGalvo:
    """Test galvo scanner control."""

    def test_move_galvo_center(self):
        """Test moving galvo to center."""
        ctrl = LaserController()
        ctrl.move_galvo(0.0, 0.0)
        assert ctrl._current_position == (0.0, 0.0)

    def test_move_galvo_positive_angles(self):
        """Test moving galvo with positive angles."""
        ctrl = LaserController()
        ctrl.move_galvo(10.5, 15.3)
        assert ctrl._current_position == (10.5, 15.3)

    def test_move_galvo_negative_angles(self):
        """Test moving galvo with negative angles."""
        ctrl = LaserController()
        ctrl.move_galvo(-10.5, -15.3)
        assert ctrl._current_position == (-10.5, -15.3)

    def test_move_galvo_max_positive(self):
        """Test moving galvo to maximum positive angle."""
        ctrl = LaserController()
        ctrl.move_galvo(20.0, 20.0)
        assert ctrl._current_position == (20.0, 20.0)

    def test_move_galvo_max_negative(self):
        """Test moving galvo to maximum negative angle."""
        ctrl = LaserController()
        ctrl.move_galvo(-20.0, -20.0)
        assert ctrl._current_position == (-20.0, -20.0)

    def test_move_galvo_x_out_of_range_high(self):
        """Test X angle above maximum."""
        ctrl = LaserController()
        with pytest.raises(ValueError, match="X angle out of range"):
            ctrl.move_galvo(20.1, 0.0)

    def test_move_galvo_x_out_of_range_low(self):
        """Test X angle below minimum."""
        ctrl = LaserController()
        with pytest.raises(ValueError, match="X angle out of range"):
            ctrl.move_galvo(-20.1, 0.0)

    def test_move_galvo_y_out_of_range_high(self):
        """Test Y angle above maximum."""
        ctrl = LaserController()
        with pytest.raises(ValueError, match="Y angle out of range"):
            ctrl.move_galvo(0.0, 20.1)

    def test_move_galvo_y_out_of_range_low(self):
        """Test Y angle below minimum."""
        ctrl = LaserController()
        with pytest.raises(ValueError, match="Y angle out of range"):
            ctrl.move_galvo(0.0, -20.1)


class TestLaserControllerStatus:
    """Test status reporting."""

    def test_get_status_off(self):
        """Test status when laser is off."""
        ctrl = LaserController()
        status = ctrl.get_status()
        assert status["mode"] == LaserMode.OFF.value
        assert status["power_w"] == 0.0
        assert status["power_max_w"] == 8.0
        assert status["wavelength_nm"] == 532
        assert status["scanning"] is False
        assert status["galvo_position_deg"]["x"] == 0.0
        assert status["galvo_position_deg"]["y"] == 0.0

    def test_get_status_running(self):
        """Test status when laser is running."""
        ctrl = LaserController()
        ctrl.set_power(5.0)
        ctrl.start_continuous()
        ctrl.move_galvo(10.0, -5.0)

        status = ctrl.get_status()
        assert status["mode"] == LaserMode.CONTINUOUS.value
        assert status["power_w"] == 5.0
        assert status["scanning"] is False
        assert status["galvo_position_deg"]["x"] == 10.0
        assert status["galvo_position_deg"]["y"] == -5.0

    def test_get_status_scanning(self):
        """Test status when scanning."""
        ctrl = LaserController()
        ctrl.set_power(3.0)
        ctrl.start_scan()

        status = ctrl.get_status()
        assert status["mode"] == LaserMode.SCANNING.value
        assert status["scanning"] is True
        assert status["power_w"] == 3.0

    def test_get_telemetry(self):
        """Test telemetry reporting."""
        ctrl = LaserController()
        ctrl.set_power(6.0)
        ctrl.start_continuous()

        telemetry = ctrl.get_telemetry()
        assert telemetry["mode"] == LaserMode.CONTINUOUS.value
        assert telemetry["power_w"] == 6.0
        assert telemetry["scanning"] is False


class TestLaserControllerConstants:
    """Test physical constants."""

    def test_laser_power_max_constant(self):
        """Test maximum laser power constant."""
        assert LaserController.LASER_POWER_MAX_W == 8.0

    def test_laser_wavelength_constant(self):
        """Test laser wavelength constant."""
        assert LaserController.LASER_WAVELENGTH_NM == 532

    def test_galvo_max_angle_constant(self):
        """Test galvo maximum angle constant."""
        assert LaserController.GALVO_MAX_ANGLE_DEG == 20.0

    def test_galvo_step_response_constant(self):
        """Test galvo step response time constant."""
        assert LaserController.GALVO_STEP_RESPONSE_US == 500


class TestScanPoint:
    """Test ScanPoint dataclass."""

    def test_scan_point_creation(self):
        """Test creating a scan point."""
        point = ScanPoint(x_angle_deg=10.0, y_angle_deg=-5.0, power_fraction=0.8)
        assert point.x_angle_deg == 10.0
        assert point.y_angle_deg == -5.0
        assert point.power_fraction == 0.8

    def test_scan_point_requires_all_fields(self):
        """Test that ScanPoint requires all fields (no defaults)."""
        with pytest.raises(TypeError):
            ScanPoint()


class TestLaserControllerScanVoxelGrid:
    """Test voxel grid scanning."""

    def test_scan_voxel_grid_not_active(self):
        """Test scanning when not in scan mode."""
        ctrl = LaserController()
        voxel_grid = [[[]]]  # Placeholder grid
        # Should not raise, just log warning
        ctrl.scan_voxel_grid(voxel_grid)

    def test_scan_voxel_grid_active(self):
        """Test scanning in active scan mode."""
        ctrl = LaserController()
        ctrl.start_scan()
        voxel_grid = [[[]]]
        # Should not raise
        ctrl.scan_voxel_grid(voxel_grid, frame_rate_fps=30)

    def test_scan_voxel_grid_with_custom_frame_rate(self):
        """Test scanning with custom frame rate."""
        ctrl = LaserController()
        ctrl.start_scan()
        voxel_grid = [[[]]]
        ctrl.scan_voxel_grid(voxel_grid, frame_rate_fps=60)
        # Verify no errors
        assert ctrl._scan_active is True
