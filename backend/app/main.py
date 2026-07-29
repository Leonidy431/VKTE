"""
FastAPI application for Volumetric Display control.

References:
- VOLUMETRIC_DISPLAY_SPECIFICATION.md
- docs/BUBBLE_TECHNOLOGY.md
- docs/HARDWARE_BOM.md
"""

from fastapi import FastAPI, HTTPException, WebSocket
from fastapi.middleware.cors import CORSMiddleware
from contextlib import asynccontextmanager
import logging
import asyncio
from typing import Optional

from app.modules.volumetric.bubble_generator import BubbleGenerator
from app.modules.volumetric.laser_controller import LaserController
from app.modules.volumetric.volumetric_renderer import VolumetricRenderer
from app.modules.volumetric.hud_renderer import HUDRenderer, HUDMode, TelemetryFrame
from app.config import Settings

logger = logging.getLogger(__name__)
settings = Settings()

# Global instances (initialized at startup)
bubble_gen: Optional[BubbleGenerator] = None
laser_ctrl: Optional[LaserController] = None
renderer: Optional[VolumetricRenderer] = None
hud_renderer: Optional[HUDRenderer] = None


@asynccontextmanager
async def lifespan(app: FastAPI):
    """Startup and shutdown logic for the application."""
    global bubble_gen, laser_ctrl, renderer, hud_renderer

    logger.info("Initializing Volumetric Display subsystems...")

    try:
        # Phase 5: Prototyping (initialize with mock hardware for dev)
        bubble_gen = BubbleGenerator(
            frequency_hz=settings.BUBBLE_FREQUENCY,
            duty_cycle=settings.BUBBLE_DUTY_CYCLE,
            stabilizer_type=settings.STABILIZER_AGENT,
            use_mock_hw=settings.MOCK_HARDWARE,
        )

        laser_ctrl = LaserController(
            wavelength_nm=settings.LASER_WAVELENGTH,
            power_w=settings.LASER_POWER,
            use_mock_hw=settings.MOCK_HARDWARE,
        )

        renderer = VolumetricRenderer(
            grid_resolution=settings.VOXEL_RESOLUTION,
            frame_rate_fps=settings.FRAME_RATE,
        )

        hud_renderer = HUDRenderer(
            mode=HUDMode.LAND,
            brightness_percent=100.0,
        )

        logger.info("Subsystems initialized successfully.")
    except Exception as e:
        logger.error(f"Failed to initialize subsystems: {e}", exc_info=True)
        raise

    yield

    logger.info("Shutting down Volumetric Display subsystems...")
    try:
        if bubble_gen:
            bubble_gen.stop()
        if laser_ctrl:
            laser_ctrl.shutdown()
    except Exception as e:
        logger.error(f"Error during shutdown: {e}", exc_info=True)


app = FastAPI(
    title="Volumetric Display Control API",
    description="REST + WebSocket API for laser+bubble volumetric rendering",
    version="0.1.0",
    lifespan=lifespan,
)

# CORS configuration
app.add_middleware(
    CORSMiddleware,
    allow_origins=settings.CORS_ORIGINS,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


# ============================================================================
# Health & Status Endpoints
# ============================================================================

@app.get("/health")
async def health_check():
    """Basic health check endpoint."""
    return {"status": "healthy"}


@app.get("/api/v1/status")
async def system_status():
    """Get full system status (hardware, software, temperatures)."""
    if not all([bubble_gen, laser_ctrl, renderer]):
        raise HTTPException(status_code=503, detail="Subsystems not initialized")

    return {
        "bubble_generator": bubble_gen.get_status(),
        "laser_controller": laser_ctrl.get_status(),
        "renderer": renderer.get_status(),
        "timestamp": asyncio.get_event_loop().time(),
    }


# ============================================================================
# Bubble Control Endpoints
# ============================================================================

@app.post("/api/v1/bubble/start")
async def start_bubbles(frequency_hz: int = 40000, duty_cycle: float = 0.5):
    """Start bubble generation at specified frequency and duty cycle."""
    if not bubble_gen:
        raise HTTPException(status_code=503, detail="Bubble generator not initialized")

    try:
        bubble_gen.set_frequency(frequency_hz)
        bubble_gen.set_duty_cycle(duty_cycle)
        bubble_gen.start()
        return {"status": "bubbles_started", "frequency": frequency_hz, "duty_cycle": duty_cycle}
    except ValueError as e:
        raise HTTPException(status_code=400, detail=str(e))


@app.post("/api/v1/bubble/stop")
async def stop_bubbles():
    """Stop bubble generation."""
    if not bubble_gen:
        raise HTTPException(status_code=503, detail="Bubble generator not initialized")

    bubble_gen.stop()
    return {"status": "bubbles_stopped"}


# ============================================================================
# Laser Control Endpoints
# ============================================================================

@app.post("/api/v1/laser/power")
async def set_laser_power(power_w: float):
    """Set laser output power in watts."""
    if not laser_ctrl:
        raise HTTPException(status_code=503, detail="Laser controller not initialized")

    if not (0 <= power_w <= settings.LASER_POWER):
        raise HTTPException(status_code=400, detail=f"Power out of range [0, {settings.LASER_POWER}]")

    laser_ctrl.set_power(power_w)
    return {"status": "laser_power_set", "power_w": power_w}


@app.post("/api/v1/laser/scan")
async def start_laser_scan():
    """Start laser scanning."""
    if not laser_ctrl:
        raise HTTPException(status_code=503, detail="Laser controller not initialized")

    laser_ctrl.start_scan()
    return {"status": "laser_scan_started"}


# ============================================================================
# Render Endpoints
# ============================================================================

@app.post("/api/v1/render/object")
async def render_3d_object(object_type: str, scale: float = 1.0):
    """
    Render a 3D object type in the volumetric display.

    Supported types: sphere, cube, torus, mesh_custom
    """
    if not renderer:
        raise HTTPException(status_code=503, detail="Renderer not initialized")

    try:
        renderer.render_object(object_type, scale)
        return {"status": "render_started", "object": object_type, "scale": scale}
    except ValueError as e:
        raise HTTPException(status_code=400, detail=str(e))


# ============================================================================
# HUD Control Endpoints
# ============================================================================

@app.post("/api/v1/hud/render")
async def render_hud_frame(
    speed_kmh: float = 0.0,
    engine_temp_c: float = 20.0,
    battery_voltage_v: float = 12.0,
    engine_current_a: float = 0.0,
    depth_m: float = 0.0,
    water_temp_c: float = 15.0,
    pressure_bar: float = 1.0,
    salinity_ppt: float = 35.0,
):
    """
    Render HUD frame with telemetry data.

    Supports both Land (Volga automotive) and Marine (underwater) modes.
    Returns metadata about the rendered frame.
    """
    if not hud_renderer:
        raise HTTPException(status_code=503, detail="HUD renderer not initialized")

    telemetry = TelemetryFrame(
        speed_kmh=speed_kmh,
        engine_temp_c=engine_temp_c,
        battery_voltage_v=battery_voltage_v,
        engine_current_a=engine_current_a,
        depth_m=depth_m,
        water_temp_c=water_temp_c,
        pressure_bar=pressure_bar,
        salinity_ppt=salinity_ppt,
        mode=hud_renderer.mode,
    )

    result = hud_renderer.render_frame(telemetry)
    return result


@app.post("/api/v1/hud/mode")
async def set_hud_mode(mode: str):
    """
    Set HUD operating mode: land, marine, debug, off

    - land: Volga 2410 automotive display (speedometer, engine status, battery)
    - marine: Underwater display (depth gauge, water temp, pressure, salinity)
    - debug: Simulator display (all telemetry at once)
    - off: Display off
    """
    if not hud_renderer:
        raise HTTPException(status_code=503, detail="HUD renderer not initialized")

    try:
        hud_mode = HUDMode(mode.lower())
        hud_renderer.set_mode(hud_mode)
        return {"status": "hud_mode_set", "mode": hud_mode.value}
    except ValueError:
        raise HTTPException(
            status_code=400,
            detail=f"Invalid mode '{mode}'. Must be: land, marine, debug, off"
        )


@app.post("/api/v1/hud/brightness")
async def set_hud_brightness(percent: float):
    """Set HUD display brightness (0-100%)."""
    if not hud_renderer:
        raise HTTPException(status_code=503, detail="HUD renderer not initialized")

    try:
        hud_renderer.set_brightness(percent)
        return {
            "status": "hud_brightness_set",
            "brightness_percent": percent,
            "brightness_cd_m2": int(4000 * percent / 100),  # DLP TRP-4500 spec: 4000 cd/m²
        }
    except ValueError as e:
        raise HTTPException(status_code=400, detail=str(e))


@app.get("/api/v1/hud/status")
async def get_hud_status():
    """Get current HUD status."""
    if not hud_renderer:
        raise HTTPException(status_code=503, detail="HUD renderer not initialized")

    return hud_renderer.get_status()


# ============================================================================
# WebSocket for Real-time Telemetry
# ============================================================================

@app.websocket("/ws/telemetry")
async def telemetry_websocket(websocket: WebSocket):
    """
    WebSocket for streaming real-time telemetry:
    - Laser power, scan frequency, frame rate
    - Bubble density, resonant frequency, cavitation threshold
    - HUD status (brightness, mode, frame count)
    - Temperature, system health
    """
    await websocket.accept()
    logger.info("Telemetry WebSocket connected")

    try:
        while True:
            # Send telemetry every 100ms
            if all([bubble_gen, laser_ctrl, renderer, hud_renderer]):
                telemetry = {
                    "laser": laser_ctrl.get_telemetry(),
                    "bubbles": bubble_gen.get_telemetry(),
                    "render": renderer.get_telemetry(),
                    "hud": hud_renderer.get_status(),
                }
                await websocket.send_json(telemetry)

            await asyncio.sleep(0.1)
    except Exception as e:
        logger.error(f"WebSocket error: {e}")
    finally:
        await websocket.close()


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(
        "app.main:app",
        host=settings.HOST,
        port=settings.PORT,
        reload=settings.DEBUG,
    )
