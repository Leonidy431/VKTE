"""
Request/Response schemas with validation.

All API inputs go through Pydantic validation to catch invalid/malicious data early.
"""

from pydantic import BaseModel, Field, validator
from typing import List, Optional
import math


class HUDRenderRequest(BaseModel):
    """Validated HUD render request."""
    speed_kmh: float = Field(
        default=0.0,
        ge=0,
        le=250,
        description="Vehicle speed 0-250 km/h (negative/NaN rejected)"
    )
    engine_temp_c: float = Field(
        default=20.0,
        ge=-40,
        le=150,
        description="Engine temperature -40 to +150°C"
    )
    battery_voltage_v: float = Field(
        default=12.0,
        ge=8.0,
        le=16.0,
        description="Battery voltage 8-16V (automotive 12V nominal)"
    )
    engine_current_a: float = Field(
        default=0.0,
        ge=-100,
        le=200,
        description="Engine current -100 to +200A (charge/discharge)"
    )

    # Marine mode parameters
    depth_m: float = Field(
        default=0.0,
        ge=-5,  # Small negative margin for sensor error
        le=500,
        description="Underwater depth 0-500m"
    )
    water_temp_c: float = Field(
        default=15.0,
        ge=-2,  # Seawater freeze point
        le=40,
        description="Water temperature -2 to +40°C"
    )
    pressure_bar: float = Field(
        default=1.0,
        ge=0.9,
        le=150,
        description="Pressure 0.9-150 bar (1 atm @ surface, 1 atm per 10m depth)"
    )
    salinity_ppt: float = Field(
        default=35.0,
        ge=0,
        le=40,
        description="Salinity 0-40 PSU (seawater ~35)"
    )

    @validator('speed_kmh', 'engine_temp_c', 'battery_voltage_v', 'engine_current_a',
               'depth_m', 'water_temp_c', 'pressure_bar', 'salinity_ppt')
    def validate_finite(cls, v):
        """Reject NaN and infinite values."""
        if math.isnan(v) or math.isinf(v):
            raise ValueError(f'Value must be finite, got {v}')
        return v


class LaserPowerRequest(BaseModel):
    """Validated laser power request."""
    power_w: float = Field(
        ge=0.0,
        le=8.0,
        description="Laser power 0-8W (CNI MGL-III-532 max)"
    )

    @validator('power_w')
    def validate_finite(cls, v):
        if math.isnan(v) or math.isinf(v):
            raise ValueError(f'Power must be finite, got {v}')
        return v


class HUDBrightnessRequest(BaseModel):
    """Validated HUD brightness request."""
    percent: float = Field(
        ge=0.0,
        le=100.0,
        description="Display brightness 0-100%"
    )

    @validator('percent')
    def validate_finite(cls, v):
        if math.isnan(v) or math.isinf(v):
            raise ValueError(f'Brightness must be finite, got {v}')
        return v


class HUDModeRequest(BaseModel):
    """Validated HUD mode request."""
    mode: str = Field(
        ...,
        regex="^(land|marine|debug|off)$",
        description="Operating mode: land, marine, debug, or off"
    )


class RenderObjectRequest(BaseModel):
    """Validated 3D object render request."""
    object_type: str = Field(
        ...,
        regex="^(sphere|cube|torus|mesh_custom)$",
        description="Object type: sphere, cube, torus, or mesh_custom"
    )
    scale: float = Field(
        default=1.0,
        ge=0.1,
        le=2.0,
        description="Scale factor 0.1-2.0"
    )

    @validator('scale')
    def validate_finite(cls, v):
        if math.isnan(v) or math.isinf(v):
            raise ValueError(f'Scale must be finite, got {v}')
        return v


class BubbleStartRequest(BaseModel):
    """Validated bubble start request."""
    frequency_hz: int = Field(
        default=40000,
        ge=20000,
        le=100000,
        description="Bubble frequency 20-100 kHz (Steminc: 40 kHz)"
    )
    duty_cycle: float = Field(
        default=0.5,
        ge=0.0,
        le=1.0,
        description="Duty cycle 0-1.0 (fraction of time active)"
    )

    @validator('duty_cycle')
    def validate_finite(cls, v):
        if math.isnan(v) or math.isinf(v):
            raise ValueError(f'Duty cycle must be finite, got {v}')
        return v


# Response schemas
class HealthResponse(BaseModel):
    status: str = "healthy"


class SystemStatusResponse(BaseModel):
    """Full system status."""
    bubble_generator: dict
    laser_controller: dict
    renderer: dict
    hud: dict
    timestamp: float


class HUDRenderResponse(BaseModel):
    """HUD render response."""
    frame: int
    mode: str
    resolution: str
    brightness_cd_m2: int
    alarms: List[str] = []
