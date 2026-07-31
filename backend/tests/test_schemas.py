"""Unit tests for Pydantic request/response schemas."""

import pytest
from pydantic import ValidationError
from app.schemas import (
    HUDRenderRequest,
    LaserPowerRequest,
    HUDBrightnessRequest,
    HUDModeRequest,
    RenderObjectRequest,
    BubbleStartRequest,
    HealthResponse,
    HUDRenderResponse,
)


class TestHUDRenderRequest:
    """Test HUDRenderRequest validation."""

    def test_valid_defaults(self):
        """Test valid request with defaults."""
        req = HUDRenderRequest()
        assert req.speed_kmh == 0.0
        assert req.engine_temp_c == 20.0
        assert req.battery_voltage_v == 12.0

    def test_valid_land_mode(self):
        """Test valid land mode parameters."""
        req = HUDRenderRequest(
            speed_kmh=100.0,
            engine_temp_c=95.0,
            battery_voltage_v=13.5,
            engine_current_a=50.0,
        )
        assert req.speed_kmh == 100.0
        assert req.engine_current_a == 50.0

    def test_valid_marine_mode(self):
        """Test valid marine mode parameters."""
        req = HUDRenderRequest(
            depth_m=30.0, water_temp_c=18.0, pressure_bar=4.0, salinity_ppt=35.0
        )
        assert req.depth_m == 30.0
        assert req.water_temp_c == 18.0

    def test_speed_min_boundary(self):
        """Test speed at minimum boundary."""
        req = HUDRenderRequest(speed_kmh=0.0)
        assert req.speed_kmh == 0.0

    def test_speed_max_boundary(self):
        """Test speed at maximum boundary."""
        req = HUDRenderRequest(speed_kmh=250.0)
        assert req.speed_kmh == 250.0

    def test_speed_below_min(self):
        """Test speed below minimum."""
        with pytest.raises(ValidationError):
            HUDRenderRequest(speed_kmh=-0.1)

    def test_speed_above_max(self):
        """Test speed above maximum."""
        with pytest.raises(ValidationError):
            HUDRenderRequest(speed_kmh=250.1)

    def test_speed_nan_rejected(self):
        """Test that NaN speed is rejected."""
        with pytest.raises(ValidationError):
            HUDRenderRequest(speed_kmh=float("nan"))

    def test_speed_inf_rejected(self):
        """Test that infinite speed is rejected."""
        with pytest.raises(ValidationError):
            HUDRenderRequest(speed_kmh=float("inf"))

    def test_speed_neg_inf_rejected(self):
        """Test that negative infinite speed is rejected."""
        with pytest.raises(ValidationError):
            HUDRenderRequest(speed_kmh=float("-inf"))

    def test_engine_temp_min_boundary(self):
        """Test engine temperature at minimum boundary."""
        req = HUDRenderRequest(engine_temp_c=-40.0)
        assert req.engine_temp_c == -40.0

    def test_engine_temp_max_boundary(self):
        """Test engine temperature at maximum boundary."""
        req = HUDRenderRequest(engine_temp_c=150.0)
        assert req.engine_temp_c == 150.0

    def test_engine_temp_below_min(self):
        """Test engine temperature below minimum."""
        with pytest.raises(ValidationError):
            HUDRenderRequest(engine_temp_c=-40.1)

    def test_engine_temp_above_max(self):
        """Test engine temperature above maximum."""
        with pytest.raises(ValidationError):
            HUDRenderRequest(engine_temp_c=150.1)

    def test_battery_voltage_min_boundary(self):
        """Test battery voltage at minimum boundary."""
        req = HUDRenderRequest(battery_voltage_v=8.0)
        assert req.battery_voltage_v == 8.0

    def test_battery_voltage_max_boundary(self):
        """Test battery voltage at maximum boundary."""
        req = HUDRenderRequest(battery_voltage_v=16.0)
        assert req.battery_voltage_v == 16.0

    def test_battery_voltage_below_min(self):
        """Test battery voltage below minimum."""
        with pytest.raises(ValidationError):
            HUDRenderRequest(battery_voltage_v=7.9)

    def test_battery_voltage_above_max(self):
        """Test battery voltage above maximum."""
        with pytest.raises(ValidationError):
            HUDRenderRequest(battery_voltage_v=16.1)

    def test_depth_min_boundary(self):
        """Test depth at minimum boundary (small negative for sensor error)."""
        req = HUDRenderRequest(depth_m=-5.0)
        assert req.depth_m == -5.0

    def test_depth_max_boundary(self):
        """Test depth at maximum boundary."""
        req = HUDRenderRequest(depth_m=500.0)
        assert req.depth_m == 500.0

    def test_depth_below_min(self):
        """Test depth below minimum."""
        with pytest.raises(ValidationError):
            HUDRenderRequest(depth_m=-5.1)

    def test_depth_above_max(self):
        """Test depth above maximum."""
        with pytest.raises(ValidationError):
            HUDRenderRequest(depth_m=500.1)

    def test_pressure_min_boundary(self):
        """Test pressure at minimum boundary."""
        req = HUDRenderRequest(pressure_bar=0.9)
        assert req.pressure_bar == 0.9

    def test_pressure_max_boundary(self):
        """Test pressure at maximum boundary."""
        req = HUDRenderRequest(pressure_bar=150.0)
        assert req.pressure_bar == 150.0

    def test_pressure_below_min(self):
        """Test pressure below minimum."""
        with pytest.raises(ValidationError):
            HUDRenderRequest(pressure_bar=0.89)

    def test_pressure_above_max(self):
        """Test pressure above maximum."""
        with pytest.raises(ValidationError):
            HUDRenderRequest(pressure_bar=150.1)

    def test_salinity_min_boundary(self):
        """Test salinity at minimum boundary."""
        req = HUDRenderRequest(salinity_ppt=0.0)
        assert req.salinity_ppt == 0.0

    def test_salinity_max_boundary(self):
        """Test salinity at maximum boundary."""
        req = HUDRenderRequest(salinity_ppt=40.0)
        assert req.salinity_ppt == 40.0

    def test_salinity_below_min(self):
        """Test salinity below minimum."""
        with pytest.raises(ValidationError):
            HUDRenderRequest(salinity_ppt=-0.1)

    def test_salinity_above_max(self):
        """Test salinity above maximum."""
        with pytest.raises(ValidationError):
            HUDRenderRequest(salinity_ppt=40.1)


class TestLaserPowerRequest:
    """Test LaserPowerRequest validation."""

    def test_valid_power(self):
        """Test valid laser power."""
        req = LaserPowerRequest(power_w=5.0)
        assert req.power_w == 5.0

    def test_power_min_boundary(self):
        """Test power at minimum boundary."""
        req = LaserPowerRequest(power_w=0.0)
        assert req.power_w == 0.0

    def test_power_max_boundary(self):
        """Test power at maximum boundary."""
        req = LaserPowerRequest(power_w=8.0)
        assert req.power_w == 8.0

    def test_power_below_min(self):
        """Test power below minimum."""
        with pytest.raises(ValidationError):
            LaserPowerRequest(power_w=-0.1)

    def test_power_above_max(self):
        """Test power above maximum."""
        with pytest.raises(ValidationError):
            LaserPowerRequest(power_w=8.1)

    def test_power_nan_rejected(self):
        """Test that NaN power is rejected."""
        with pytest.raises(ValidationError):
            LaserPowerRequest(power_w=float("nan"))

    def test_power_inf_rejected(self):
        """Test that infinite power is rejected."""
        with pytest.raises(ValidationError):
            LaserPowerRequest(power_w=float("inf"))


class TestHUDBrightnessRequest:
    """Test HUDBrightnessRequest validation."""

    def test_valid_brightness(self):
        """Test valid brightness."""
        req = HUDBrightnessRequest(percent=50.0)
        assert req.percent == 50.0

    def test_brightness_min_boundary(self):
        """Test brightness at minimum boundary."""
        req = HUDBrightnessRequest(percent=0.0)
        assert req.percent == 0.0

    def test_brightness_max_boundary(self):
        """Test brightness at maximum boundary."""
        req = HUDBrightnessRequest(percent=100.0)
        assert req.percent == 100.0

    def test_brightness_below_min(self):
        """Test brightness below minimum."""
        with pytest.raises(ValidationError):
            HUDBrightnessRequest(percent=-0.1)

    def test_brightness_above_max(self):
        """Test brightness above maximum."""
        with pytest.raises(ValidationError):
            HUDBrightnessRequest(percent=100.1)

    def test_brightness_nan_rejected(self):
        """Test that NaN brightness is rejected."""
        with pytest.raises(ValidationError):
            HUDBrightnessRequest(percent=float("nan"))


class TestHUDModeRequest:
    """Test HUDModeRequest validation."""

    def test_valid_land_mode(self):
        """Test valid land mode."""
        req = HUDModeRequest(mode="land")
        assert req.mode == "land"

    def test_valid_marine_mode(self):
        """Test valid marine mode."""
        req = HUDModeRequest(mode="marine")
        assert req.mode == "marine"

    def test_valid_debug_mode(self):
        """Test valid debug mode."""
        req = HUDModeRequest(mode="debug")
        assert req.mode == "debug"

    def test_valid_off_mode(self):
        """Test valid off mode."""
        req = HUDModeRequest(mode="off")
        assert req.mode == "off"

    def test_invalid_mode(self):
        """Test invalid mode."""
        with pytest.raises(ValidationError):
            HUDModeRequest(mode="invalid")

    def test_mode_case_sensitive(self):
        """Test that mode is case-sensitive."""
        with pytest.raises(ValidationError):
            HUDModeRequest(mode="LAND")

    def test_empty_mode(self):
        """Test empty mode."""
        with pytest.raises(ValidationError):
            HUDModeRequest(mode="")


class TestRenderObjectRequest:
    """Test RenderObjectRequest validation."""

    def test_valid_sphere(self):
        """Test valid sphere object."""
        req = RenderObjectRequest(object_type="sphere")
        assert req.object_type == "sphere"
        assert req.scale == 1.0

    def test_valid_cube(self):
        """Test valid cube object."""
        req = RenderObjectRequest(object_type="cube", scale=1.5)
        assert req.object_type == "cube"
        assert req.scale == 1.5

    def test_valid_torus(self):
        """Test valid torus object."""
        req = RenderObjectRequest(object_type="torus", scale=0.5)
        assert req.object_type == "torus"

    def test_valid_mesh_custom(self):
        """Test valid mesh_custom object."""
        req = RenderObjectRequest(object_type="mesh_custom")
        assert req.object_type == "mesh_custom"

    def test_invalid_object_type(self):
        """Test invalid object type."""
        with pytest.raises(ValidationError):
            RenderObjectRequest(object_type="invalid")

    def test_scale_min_boundary(self):
        """Test scale at minimum boundary."""
        req = RenderObjectRequest(object_type="sphere", scale=0.1)
        assert req.scale == 0.1

    def test_scale_max_boundary(self):
        """Test scale at maximum boundary."""
        req = RenderObjectRequest(object_type="sphere", scale=2.0)
        assert req.scale == 2.0

    def test_scale_below_min(self):
        """Test scale below minimum."""
        with pytest.raises(ValidationError):
            RenderObjectRequest(object_type="sphere", scale=0.09)

    def test_scale_above_max(self):
        """Test scale above maximum."""
        with pytest.raises(ValidationError):
            RenderObjectRequest(object_type="sphere", scale=2.1)

    def test_scale_nan_rejected(self):
        """Test that NaN scale is rejected."""
        with pytest.raises(ValidationError):
            RenderObjectRequest(object_type="sphere", scale=float("nan"))


class TestBubbleStartRequest:
    """Test BubbleStartRequest validation."""

    def test_valid_defaults(self):
        """Test valid defaults."""
        req = BubbleStartRequest()
        assert req.frequency_hz == 40000
        assert req.duty_cycle == 0.5

    def test_valid_custom_frequency(self):
        """Test valid custom frequency."""
        req = BubbleStartRequest(frequency_hz=50000)
        assert req.frequency_hz == 50000

    def test_frequency_min_boundary(self):
        """Test frequency at minimum boundary."""
        req = BubbleStartRequest(frequency_hz=20000)
        assert req.frequency_hz == 20000

    def test_frequency_max_boundary(self):
        """Test frequency at maximum boundary (BubbleGenerator's 80 kHz hardware bound)."""
        req = BubbleStartRequest(frequency_hz=80000)
        assert req.frequency_hz == 80000

    def test_frequency_below_min(self):
        """Test frequency below minimum."""
        with pytest.raises(ValidationError):
            BubbleStartRequest(frequency_hz=19999)

    def test_frequency_above_max(self):
        """Test frequency above maximum."""
        with pytest.raises(ValidationError):
            BubbleStartRequest(frequency_hz=80001)

    def test_duty_cycle_min_boundary(self):
        """Test duty cycle at minimum boundary."""
        req = BubbleStartRequest(duty_cycle=0.0)
        assert req.duty_cycle == 0.0

    def test_duty_cycle_max_boundary(self):
        """Test duty cycle at maximum boundary."""
        req = BubbleStartRequest(duty_cycle=1.0)
        assert req.duty_cycle == 1.0

    def test_duty_cycle_below_min(self):
        """Test duty cycle below minimum."""
        with pytest.raises(ValidationError):
            BubbleStartRequest(duty_cycle=-0.1)

    def test_duty_cycle_above_max(self):
        """Test duty cycle above maximum."""
        with pytest.raises(ValidationError):
            BubbleStartRequest(duty_cycle=1.1)

    def test_duty_cycle_nan_rejected(self):
        """Test that NaN duty cycle is rejected."""
        with pytest.raises(ValidationError):
            BubbleStartRequest(duty_cycle=float("nan"))


class TestResponseSchemas:
    """Test response schemas."""

    def test_health_response_default(self):
        """Test HealthResponse default."""
        resp = HealthResponse()
        assert resp.status == "healthy"

    def test_health_response_json(self):
        """Test HealthResponse can be converted to JSON."""
        resp = HealthResponse()
        json_data = resp.model_dump()
        assert json_data["status"] == "healthy"

    def test_hud_render_response_creation(self):
        """Test HUDRenderResponse creation."""
        resp = HUDRenderResponse(
            frame=1,
            mode="land",
            resolution="1280x800",
            brightness_cd_m2=4000,
            alarms=[],
        )
        assert resp.frame == 1
        assert resp.mode == "land"
        assert resp.brightness_cd_m2 == 4000

    def test_hud_render_response_with_alarms(self):
        """Test HUDRenderResponse with alarms."""
        resp = HUDRenderResponse(
            frame=5,
            mode="marine",
            resolution="1280x800",
            brightness_cd_m2=2000,
            alarms=["overtemp", "low_voltage"],
        )
        assert len(resp.alarms) == 2
        assert "overtemp" in resp.alarms
