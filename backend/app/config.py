"""
Configuration settings for Volumetric Display backend.

Load from environment variables or .env file.
"""

from pydantic_settings import BaseSettings
from typing import List


class Settings(BaseSettings):
    """Application configuration."""

    # Server
    HOST: str = "0.0.0.0"
    PORT: int = 8000
    DEBUG: bool = True

    # Security (from environment or default)
    API_KEY: str = "sk-vkte-dev-change-in-production"  # CHANGE THIS IN PRODUCTION

    # CORS
    CORS_ORIGINS: List[str] = ["http://localhost:3000", "http://localhost:8080"]

    # Hardware: Laser (from HARDWARE_BOM.md)
    LASER_WAVELENGTH: int = 532  # nm (green DPSS, CNI MGL-III-532)
    LASER_POWER: float = 8.0  # watts
    LASER_MODULATION_HZ: int = 100000  # TTL/analog modulation frequency

    # Hardware: Bubble generation (from HARDWARE_BOM.md)
    BUBBLE_FREQUENCY: int = 40000  # Hz (Steminc transducer 40 kHz)
    BUBBLE_DUTY_CYCLE: float = 0.5  # 0–1.0
    STABILIZER_AGENT: str = "saponin"  # or "SDS", "phospholipid"

    # Hardware: Galvo scanner
    GALVO_STEP_RESPONSE_US: int = 500  # microseconds (Cambridge Tech 6215H)
    GALVO_SCAN_ANGLE_DEG: float = 20.0  # ±20° optical

    # Rendering
    VOXEL_RESOLUTION: int = 100  # 100×100×100 voxel grid
    FRAME_RATE: int = 30  # FPS (Phase 1: 10-60 FPS range, start at 30)

    # Hardware simulation (for dev/testing without real hardware)
    MOCK_HARDWARE: bool = True

    # Logging
    LOG_LEVEL: str = "INFO"
    LOG_FILE: str = "/tmp/volumetric_display.log"

    class Config:
        env_file = ".env"
        case_sensitive = True


settings = Settings()
