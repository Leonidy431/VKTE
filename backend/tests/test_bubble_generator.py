"""Unit tests for BubbleGenerator module."""

import pytest
import math
from app.modules.volumetric.bubble_generator import BubbleGenerator, BubbleStabilityRegime, BubbleMetrics


class TestBubbleGeneratorInit:
    """Test BubbleGenerator initialization."""

    def test_init_defaults(self):
        """Test default initialization."""
        gen = BubbleGenerator()
        assert gen.frequency_hz == 40000
        assert gen.duty_cycle == 0.5
        assert gen.stabilizer_type == "saponin"
        assert gen.use_mock_hw is True
        assert gen._running is False

    def test_init_custom_params(self):
        """Test initialization with custom parameters."""
        gen = BubbleGenerator(
            frequency_hz=50000,
            duty_cycle=0.7,
            stabilizer_type="SDS",
            use_mock_hw=False
        )
        assert gen.frequency_hz == 50000
        assert gen.duty_cycle == 0.7
        assert gen.stabilizer_type == "SDS"
        assert gen.use_mock_hw is False


class TestBubbleGeneratorFrequency:
    """Test frequency control."""

    def test_set_frequency_valid(self):
        """Test setting valid frequency."""
        gen = BubbleGenerator()
        gen.set_frequency(45000)
        assert gen.frequency_hz == 45000

    def test_set_frequency_out_of_range_low(self):
        """Test setting frequency below minimum."""
        gen = BubbleGenerator()
        with pytest.raises(ValueError, match="Frequency"):
            gen.set_frequency(5000)

    def test_set_frequency_out_of_range_high(self):
        """Test setting frequency above maximum."""
        gen = BubbleGenerator()
        with pytest.raises(ValueError, match="Frequency"):
            gen.set_frequency(100000)

    def test_resonant_frequency_calculation(self):
        """Test Minnaert resonant frequency calculation."""
        gen = BubbleGenerator()
        gen.set_bubble_radius(50)  # Set radius first
        status = gen.get_status()
        resonant_freq = status["resonant_frequency_hz"]
        # Minnaert formula for 50µm bubble in seawater: ~65 kHz
        assert 50000 < resonant_freq < 80000

    def test_resonant_frequency_scales_inversely_with_radius(self):
        """Test that larger bubbles have lower resonant frequency."""
        gen = BubbleGenerator()
        gen.set_bubble_radius(20)
        small_freq = gen.get_status()["resonant_frequency_hz"]

        gen.set_bubble_radius(100)
        large_freq = gen.get_status()["resonant_frequency_hz"]

        assert small_freq > large_freq


class TestBubbleGeneratorDutyCycle:
    """Test duty cycle control."""

    def test_set_duty_cycle_valid(self):
        """Test setting valid duty cycle."""
        gen = BubbleGenerator()
        gen.set_duty_cycle(0.3)
        assert gen.duty_cycle == 0.3

    def test_set_duty_cycle_min(self):
        """Test minimum duty cycle."""
        gen = BubbleGenerator()
        gen.set_duty_cycle(0.0)
        assert gen.duty_cycle == 0.0

    def test_set_duty_cycle_max(self):
        """Test maximum duty cycle."""
        gen = BubbleGenerator()
        gen.set_duty_cycle(1.0)
        assert gen.duty_cycle == 1.0

    def test_set_duty_cycle_out_of_range(self):
        """Test duty cycle out of range."""
        gen = BubbleGenerator()
        with pytest.raises(ValueError, match="Duty cycle"):
            gen.set_duty_cycle(1.5)

        with pytest.raises(ValueError, match="Duty cycle"):
            gen.set_duty_cycle(-0.1)


class TestBubbleGeneratorStartStop:
    """Test start/stop operations."""

    def test_start(self):
        """Test starting bubble generation."""
        gen = BubbleGenerator()
        gen.start()
        assert gen._running is True

    def test_stop(self):
        """Test stopping bubble generation."""
        gen = BubbleGenerator()
        gen.start()
        gen.stop()
        assert gen._running is False

    def test_start_already_running(self):
        """Test starting when already running."""
        gen = BubbleGenerator()
        gen.start()
        # Starting again should not raise error (idempotent)
        gen.start()
        assert gen._running is True

    def test_stop_when_not_running(self):
        """Test stopping when not running."""
        gen = BubbleGenerator()
        # Stopping when not running should not raise error
        gen.stop()
        assert gen._running is False


class TestBubbleGeneratorTelemetry:
    """Test telemetry reporting."""

    def test_get_telemetry_not_running(self):
        """Test telemetry when not running."""
        gen = BubbleGenerator()
        telemetry = gen.get_telemetry()
        assert telemetry["running"] is False
        assert telemetry["density_percent"] == 0.0

    def test_get_telemetry_running(self):
        """Test telemetry when running."""
        gen = BubbleGenerator()
        gen.start()
        telemetry = gen.get_telemetry()
        assert telemetry["running"] is True
        assert "acoustic_pressure_pa" in telemetry
        assert "stability_regime" in telemetry
        assert "density_percent" in telemetry
        assert 0 <= telemetry["density_percent"] <= 100

    def test_get_status(self):
        """Test status reporting."""
        gen = BubbleGenerator()
        gen.start()
        status = gen.get_status()
        assert status["running"] is True
        assert status["stability_regime"] in [e.value for e in BubbleStabilityRegime]


class TestBubbleGeneratorAcousticPressure:
    """Test acoustic pressure settings."""

    def test_set_acoustic_pressure_valid(self):
        """Test setting valid acoustic pressure."""
        gen = BubbleGenerator()
        gen.set_acoustic_pressure(150000)  # 1.5x atmospheric
        assert gen._acoustic_amplitude_pa == 150000

    def test_set_acoustic_pressure_zero(self):
        """Test setting zero pressure (minimum)."""
        gen = BubbleGenerator()
        gen.set_acoustic_pressure(0)
        assert gen._acoustic_amplitude_pa == 0

    def test_set_acoustic_pressure_extreme(self):
        """Test extreme pressure (cavitation threshold)."""
        gen = BubbleGenerator()
        gen.set_acoustic_pressure(500000)  # 5 atm
        assert gen._acoustic_amplitude_pa == 500000

    def test_set_acoustic_pressure_out_of_range(self):
        """Test pressure above hardware maximum."""
        gen = BubbleGenerator()
        with pytest.raises(ValueError, match="Acoustic pressure"):
            gen.set_acoustic_pressure(600000)

        with pytest.raises(ValueError, match="Acoustic pressure"):
            gen.set_acoustic_pressure(-1)

    def test_cavitation_threshold_detection(self):
        """Test cavitation threshold detection."""
        gen = BubbleGenerator()
        # Below threshold
        gen.set_acoustic_pressure(50000)
        status = gen.get_status()
        assert status["stability_regime"] == BubbleStabilityRegime.OFF.value

        # Above threshold
        gen.set_acoustic_pressure(150000)
        status = gen.get_status()
        assert status["stability_regime"] != BubbleStabilityRegime.OFF.value


class TestBubbleGeneratorBubbleRadius:
    """Test bubble radius settings."""

    def test_set_bubble_radius_valid(self):
        """Test setting valid bubble radius."""
        gen = BubbleGenerator()
        gen.set_bubble_radius(100)  # 100 micrometers
        assert gen._bubble_radius_um == 100

    def test_set_bubble_radius_small(self):
        """Test small bubble radius."""
        gen = BubbleGenerator()
        gen.set_bubble_radius(10)
        assert gen._bubble_radius_um == 10

    def test_set_bubble_radius_large(self):
        """Test large bubble radius."""
        gen = BubbleGenerator()
        gen.set_bubble_radius(500)
        assert gen._bubble_radius_um == 500

    def test_set_bubble_radius_out_of_range_low(self):
        """Test radius below minimum."""
        gen = BubbleGenerator()
        with pytest.raises(ValueError, match="radius"):
            gen.set_bubble_radius(0.5)

    def test_set_bubble_radius_out_of_range_high(self):
        """Test radius above maximum."""
        gen = BubbleGenerator()
        with pytest.raises(ValueError, match="radius"):
            gen.set_bubble_radius(501)


class TestBubbleGeneratorStabilityRegime:
    """Test bubble stability regime detection."""

    def test_regime_off_when_stopped(self):
        """Test OFF regime when generator stopped."""
        gen = BubbleGenerator()
        assert gen.get_status()["stability_regime"] == BubbleStabilityRegime.OFF.value

    def test_regime_transitions(self):
        """Test stability regime transitions."""
        gen = BubbleGenerator()
        gen.start()
        gen.set_acoustic_pressure(100000)

        status = gen.get_status()
        regime = status["stability_regime"]
        assert regime in [BubbleStabilityRegime.STABLE.value, BubbleStabilityRegime.INERTIAL.value]

        # High pressure → inertial
        gen.set_acoustic_pressure(400000)
        status = gen.get_status()
        assert status["stability_regime"] in [
            BubbleStabilityRegime.INERTIAL.value,
            BubbleStabilityRegime.STABLE.value
        ]


class TestBubbleGeneratorConstants:
    """Test physical constants."""

    def test_atmospheric_pressure_constant(self):
        """Test atmospheric pressure value."""
        assert BubbleGenerator.ATMOSPHERIC_PRESSURE_PA == 101325.0

    def test_water_density_constant(self):
        """Test seawater density."""
        assert BubbleGenerator.WATER_DENSITY_KG_M3 == 1025.0

    def test_heat_capacity_ratio_constant(self):
        """Test adiabatic index (gamma)."""
        assert BubbleGenerator.HEAT_CAPACITY_RATIO == 1.4

    def test_cavitation_threshold_constant(self):
        """Test cavitation threshold."""
        assert BubbleGenerator.CAVITATION_THRESHOLD_PA == 1e5
