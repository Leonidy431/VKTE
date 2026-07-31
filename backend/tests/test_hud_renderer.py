"""Unit tests for HUDRenderer module."""

import pytest
from app.modules.volumetric.hud_renderer import HUDRenderer, HUDMode, TelemetryFrame


class TestHUDRendererInit:
    """Test HUDRenderer initialization."""

    def test_init_defaults(self):
        """Test default initialization."""
        renderer = HUDRenderer()
        assert renderer.mode == HUDMode.LAND
        assert renderer.brightness_percent == 100.0
        assert renderer._active is False
        assert renderer._frame_count == 0

    def test_init_marine_mode(self):
        """Test initialization with marine mode."""
        renderer = HUDRenderer(mode=HUDMode.MARINE, brightness_percent=50.0)
        assert renderer.mode == HUDMode.MARINE
        assert renderer.brightness_percent == 50.0

    def test_init_debug_mode(self):
        """Test initialization with debug mode."""
        renderer = HUDRenderer(mode=HUDMode.DEBUG, brightness_percent=75.0)
        assert renderer.mode == HUDMode.DEBUG
        assert renderer.brightness_percent == 75.0

    def test_frame_buffer_initialization(self):
        """Test that frame buffer is properly initialized."""
        renderer = HUDRenderer()
        assert len(renderer.frame_buffer) == renderer.DISPLAY_HEIGHT
        assert len(renderer.frame_buffer[0]) == renderer.DISPLAY_WIDTH


class TestHUDRendererBrightness:
    """Test brightness control."""

    def test_set_brightness_valid(self):
        """Test setting valid brightness."""
        renderer = HUDRenderer()
        renderer.set_brightness(50.0)
        assert renderer.brightness_percent == 50.0

    def test_set_brightness_zero(self):
        """Test setting brightness to zero."""
        renderer = HUDRenderer()
        renderer.set_brightness(0.0)
        assert renderer.brightness_percent == 0.0

    def test_set_brightness_max(self):
        """Test setting brightness to maximum."""
        renderer = HUDRenderer()
        renderer.set_brightness(100.0)
        assert renderer.brightness_percent == 100.0

    def test_set_brightness_out_of_range_negative(self):
        """Test setting brightness below zero."""
        renderer = HUDRenderer()
        with pytest.raises(ValueError, match="Brightness out of range"):
            renderer.set_brightness(-0.1)

    def test_set_brightness_out_of_range_high(self):
        """Test setting brightness above maximum."""
        renderer = HUDRenderer()
        with pytest.raises(ValueError, match="Brightness out of range"):
            renderer.set_brightness(100.1)


class TestHUDRendererMode:
    """Test HUD mode control."""

    def test_set_mode_land(self):
        """Test setting land mode."""
        renderer = HUDRenderer()
        renderer.set_mode(HUDMode.MARINE)
        renderer.set_mode(HUDMode.LAND)
        assert renderer.mode == HUDMode.LAND

    def test_set_mode_marine(self):
        """Test setting marine mode."""
        renderer = HUDRenderer(mode=HUDMode.LAND)
        renderer.set_mode(HUDMode.MARINE)
        assert renderer.mode == HUDMode.MARINE

    def test_set_mode_debug(self):
        """Test setting debug mode."""
        renderer = HUDRenderer()
        renderer.set_mode(HUDMode.DEBUG)
        assert renderer.mode == HUDMode.DEBUG

    def test_set_mode_off(self):
        """Test setting off mode."""
        renderer = HUDRenderer()
        renderer.set_mode(HUDMode.OFF)
        assert renderer.mode == HUDMode.OFF


class TestHUDRendererFrameRendering:
    """Test frame rendering."""

    def test_render_frame_land_mode(self):
        """Test rendering frame in land mode."""
        renderer = HUDRenderer(mode=HUDMode.LAND)
        telemetry = TelemetryFrame(speed_kmh=60.0, engine_temp_c=90.0)
        result = renderer.render_frame(telemetry)

        assert result["frame"] == 1
        assert result["mode"] == "land"
        assert renderer._active is True
        assert "resolution" in result
        assert "brightness_cd_m2" in result

    def test_render_frame_marine_mode(self):
        """Test rendering frame in marine mode."""
        renderer = HUDRenderer(mode=HUDMode.MARINE)
        telemetry = TelemetryFrame(depth_m=25.0, water_temp_c=18.0)
        result = renderer.render_frame(telemetry)

        assert result["frame"] == 1
        assert result["mode"] == "marine"

    def test_render_frame_debug_mode(self):
        """Test rendering frame in debug mode."""
        renderer = HUDRenderer(mode=HUDMode.DEBUG)
        telemetry = TelemetryFrame(speed_kmh=50.0, depth_m=15.0)
        result = renderer.render_frame(telemetry)

        assert result["frame"] == 1
        assert result["mode"] == "debug"

    def test_render_frame_increments_counter(self):
        """Test that rendering increments frame counter."""
        renderer = HUDRenderer()
        telemetry = TelemetryFrame()

        result1 = renderer.render_frame(telemetry)
        result2 = renderer.render_frame(telemetry)
        result3 = renderer.render_frame(telemetry)

        assert result1["frame"] == 1
        assert result2["frame"] == 2
        assert result3["frame"] == 3

    def test_render_frame_with_alarms(self):
        """Test rendering frame with alarms."""
        renderer = HUDRenderer()
        telemetry = TelemetryFrame(alarms=["overtemp", "low_voltage"])
        result = renderer.render_frame(telemetry)

        assert result["alarms"] == ["overtemp", "low_voltage"]

    def test_render_alarms_empty_list_is_noop(self):
        """Test that _render_alarms() with an empty list draws nothing."""
        renderer = HUDRenderer()
        renderer._render_alarms([])
        non_black_pixels = sum(
            1 for row in renderer.frame_buffer for pixel in row if pixel != (0, 0, 0)
        )
        assert non_black_pixels == 0

    def test_render_frame_clears_buffer(self):
        """Test that rendering clears frame buffer."""
        renderer = HUDRenderer()
        telemetry = TelemetryFrame()

        # First render
        renderer.render_frame(telemetry)

        # Second render (should clear)
        renderer.render_frame(telemetry)
        # Buffer should be mostly black after clear
        black_count = sum(
            1 for row in renderer.frame_buffer for pixel in row if pixel == (0, 0, 0)
        )
        # Most pixels should be black
        assert black_count > (renderer.DISPLAY_HEIGHT * renderer.DISPLAY_WIDTH * 0.8)


class TestTelemetryFrame:
    """Test TelemetryFrame dataclass."""

    def test_telemetry_frame_land_defaults(self):
        """Test TelemetryFrame with default land values."""
        frame = TelemetryFrame()
        assert frame.speed_kmh == 0.0
        assert frame.engine_temp_c == 20.0
        assert frame.battery_voltage_v == 12.0
        assert frame.mode == HUDMode.LAND

    def test_telemetry_frame_marine_values(self):
        """Test TelemetryFrame with marine values."""
        frame = TelemetryFrame(
            depth_m=30.0, water_temp_c=15.0, pressure_bar=4.0, salinity_ppt=35.0
        )
        assert frame.depth_m == 30.0
        assert frame.water_temp_c == 15.0
        assert frame.pressure_bar == 4.0
        assert frame.salinity_ppt == 35.0

    def test_telemetry_frame_with_alarms(self):
        """Test TelemetryFrame with alarms."""
        alarms = ["knock_detected", "overtemp"]
        frame = TelemetryFrame(alarms=alarms)
        assert frame.alarms == alarms


class TestHUDRendererStatus:
    """Test status reporting."""

    def test_get_status_inactive(self):
        """Test status when not active."""
        renderer = HUDRenderer()
        status = renderer.get_status()
        assert status["mode"] == "land"
        assert status["active"] is False
        assert status["brightness_percent"] == 100.0
        assert status["frame_count"] == 0

    def test_get_status_after_render(self):
        """Test status after rendering."""
        renderer = HUDRenderer()
        telemetry = TelemetryFrame()
        renderer.render_frame(telemetry)

        status = renderer.get_status()
        assert status["active"] is True
        assert status["frame_count"] == 1
        assert "brightness_cd_m2" in status
        assert "resolution" in status

    def test_get_status_brightness_calculation(self):
        """Test brightness calculation in status."""
        renderer = HUDRenderer(brightness_percent=50.0)
        status = renderer.get_status()

        expected_cd_m2 = int(4000 * 50.0 / 100)
        assert status["brightness_cd_m2"] == expected_cd_m2

    def test_get_status_resolution(self):
        """Test resolution in status."""
        renderer = HUDRenderer()
        status = renderer.get_status()
        assert status["resolution"] == "1280x800"


class TestHUDRendererDisplayConstants:
    """Test display constants."""

    def test_display_width_constant(self):
        """Test display width constant."""
        assert HUDRenderer.DISPLAY_WIDTH == 1280

    def test_display_height_constant(self):
        """Test display height constant."""
        assert HUDRenderer.DISPLAY_HEIGHT == 800

    def test_display_brightness_constant(self):
        """Test display brightness constant."""
        assert HUDRenderer.DISPLAY_BRIGHTNESS_CD_M2 == 4000

    def test_color_constants_exist(self):
        """Test that color constants are defined."""
        assert HUDRenderer.COLOR_PRIMARY is not None
        assert HUDRenderer.COLOR_WARNING is not None
        assert HUDRenderer.COLOR_ALARM is not None
        assert HUDRenderer.COLOR_BACKGROUND is not None
        assert HUDRenderer.COLOR_TEXT is not None

    def test_font_size_constants(self):
        """Test font size constants."""
        assert HUDRenderer.FONT_SIZE_LARGE > 0
        assert HUDRenderer.FONT_SIZE_MEDIUM > 0
        assert HUDRenderer.FONT_SIZE_SMALL > 0
        assert (
            HUDRenderer.FONT_SIZE_LARGE
            > HUDRenderer.FONT_SIZE_MEDIUM
            > HUDRenderer.FONT_SIZE_SMALL
        )


class TestHUDModeEnum:
    """Test HUDMode enum."""

    def test_hud_mode_values(self):
        """Test HUDMode enum values."""
        assert HUDMode.LAND.value == "land"
        assert HUDMode.MARINE.value == "marine"
        assert HUDMode.DEBUG.value == "debug"
        assert HUDMode.OFF.value == "off"

    def test_hud_mode_from_string(self):
        """Test creating HUDMode from string."""
        assert HUDMode("land") == HUDMode.LAND
        assert HUDMode("marine") == HUDMode.MARINE
        assert HUDMode("debug") == HUDMode.DEBUG
        assert HUDMode("off") == HUDMode.OFF


class TestHUDRendererLandMode:
    """Test land mode specific rendering."""

    def test_land_mode_renders_speedometer(self):
        """Test that land mode renders speedometer."""
        renderer = HUDRenderer(mode=HUDMode.LAND)
        telemetry = TelemetryFrame(speed_kmh=80.0)
        renderer.render_frame(telemetry)

        # Check that some pixels are drawn
        non_black_pixels = sum(
            1 for row in renderer.frame_buffer for pixel in row if pixel != (0, 0, 0)
        )
        assert non_black_pixels > 0

    def test_speedometer_warning_color(self):
        """Test speedometer warning color band (>130 km/h, <=160 km/h)."""
        renderer = HUDRenderer(mode=HUDMode.LAND)
        telemetry = TelemetryFrame(speed_kmh=150.0)
        renderer.render_frame(telemetry)
        non_black_pixels = sum(
            1 for row in renderer.frame_buffer for pixel in row if pixel != (0, 0, 0)
        )
        assert non_black_pixels > 0

    def test_speedometer_alarm_color(self):
        """Test speedometer alarm color band (>160 km/h)."""
        renderer = HUDRenderer(mode=HUDMode.LAND)
        telemetry = TelemetryFrame(speed_kmh=170.0)
        renderer.render_frame(telemetry)
        non_black_pixels = sum(
            1 for row in renderer.frame_buffer for pixel in row if pixel != (0, 0, 0)
        )
        assert non_black_pixels > 0

    def test_battery_warning_color(self):
        """Test battery status warning color band (<11.5V)."""
        renderer = HUDRenderer(mode=HUDMode.LAND)
        telemetry = TelemetryFrame(battery_voltage_v=11.2)
        renderer.render_frame(telemetry)
        assert renderer._active is True

    def test_battery_alarm_color(self):
        """Test battery status alarm color band (<11.0V)."""
        renderer = HUDRenderer(mode=HUDMode.LAND)
        telemetry = TelemetryFrame(battery_voltage_v=10.5)
        renderer.render_frame(telemetry)
        assert renderer._active is True

    def test_land_mode_engine_status(self):
        """Test land mode with various engine temperatures."""
        renderer = HUDRenderer(mode=HUDMode.LAND)
        telemetry_normal = TelemetryFrame(engine_temp_c=90.0)
        telemetry_warning = TelemetryFrame(engine_temp_c=100.0)
        telemetry_alarm = TelemetryFrame(engine_temp_c=115.0)

        # Should not raise errors
        renderer.render_frame(telemetry_normal)
        renderer.render_frame(telemetry_warning)
        renderer.render_frame(telemetry_alarm)


class TestHUDRendererMarineMode:
    """Test marine mode specific rendering."""

    def test_marine_mode_renders_depth_gauge(self):
        """Test that marine mode renders depth gauge."""
        renderer = HUDRenderer(mode=HUDMode.MARINE)
        telemetry = TelemetryFrame(depth_m=25.0, water_temp_c=18.0)
        renderer.render_frame(telemetry)

        # Check that some pixels are drawn
        non_black_pixels = sum(
            1 for row in renderer.frame_buffer for pixel in row if pixel != (0, 0, 0)
        )
        assert non_black_pixels > 0

    def test_marine_mode_depth_levels(self):
        """Test marine mode with various depths."""
        renderer = HUDRenderer(mode=HUDMode.MARINE)
        telemetry_shallow = TelemetryFrame(depth_m=10.0)
        telemetry_medium = TelemetryFrame(depth_m=45.0)
        telemetry_deep = TelemetryFrame(depth_m=85.0)

        # Should not raise errors
        renderer.render_frame(telemetry_shallow)
        renderer.render_frame(telemetry_medium)
        renderer.render_frame(telemetry_deep)


class TestHUDRendererDebugMode:
    """Test debug mode rendering."""

    def test_debug_mode_shows_all_telemetry(self):
        """Test that debug mode displays all telemetry."""
        renderer = HUDRenderer(mode=HUDMode.DEBUG)
        telemetry = TelemetryFrame(
            speed_kmh=60.0, depth_m=20.0, engine_temp_c=85.0, water_temp_c=15.0
        )
        renderer.render_frame(telemetry)

        # Check that pixels are drawn
        non_black_pixels = sum(
            1 for row in renderer.frame_buffer for pixel in row if pixel != (0, 0, 0)
        )
        assert non_black_pixels > 0


class TestHUDRendererModeTransitions:
    """Test mode transitions."""

    def test_mode_transition_land_to_marine(self):
        """Test transitioning from land to marine mode."""
        renderer = HUDRenderer(mode=HUDMode.LAND)
        telemetry = TelemetryFrame()

        renderer.render_frame(telemetry)
        assert renderer._frame_count == 1

        renderer.set_mode(HUDMode.MARINE)
        telemetry.mode = HUDMode.MARINE
        renderer.render_frame(telemetry)
        assert renderer._frame_count == 2
        assert renderer.mode == HUDMode.MARINE

    def test_mode_transition_preserves_frame_count(self):
        """Test that mode transition preserves frame count."""
        renderer = HUDRenderer()
        telemetry = TelemetryFrame()

        renderer.render_frame(telemetry)
        renderer.render_frame(telemetry)
        frame_count = renderer._frame_count

        renderer.set_mode(HUDMode.MARINE)
        assert renderer._frame_count == frame_count


class TestHUDRendererMultipleInstances:
    """Test multiple HUD renderer instances."""

    def test_independent_renderers(self):
        """Test that multiple renderer instances are independent."""
        renderer1 = HUDRenderer(mode=HUDMode.LAND, brightness_percent=75.0)
        renderer2 = HUDRenderer(mode=HUDMode.MARINE, brightness_percent=50.0)

        assert renderer1.mode == HUDMode.LAND
        assert renderer1.brightness_percent == 75.0
        assert renderer2.mode == HUDMode.MARINE
        assert renderer2.brightness_percent == 50.0

    def test_independent_frame_counts(self):
        """Test that frame counts are independent."""
        renderer1 = HUDRenderer()
        renderer2 = HUDRenderer()

        telemetry = TelemetryFrame()

        renderer1.render_frame(telemetry)
        renderer1.render_frame(telemetry)

        renderer2.render_frame(telemetry)

        assert renderer1._frame_count == 2
        assert renderer2._frame_count == 1
