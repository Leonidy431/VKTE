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
from app.config import Settings

logger = logging.getLogger(__name__)
settings = Settings()

# Global instances (initialized at startup)
bubble_gen: Optional[BubbleGenerator] = None
laser_ctrl: Optional[LaserController] = None
renderer: Optional[VolumetricRenderer] = None


@asynccontextmanager
async def lifespan(app: FastAPI):
    """Startup and shutdown logic for the application."""
    global bubble_gen, laser_ctrl, renderer

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
# WebSocket for Real-time Telemetry
# ============================================================================

@app.websocket("/ws/telemetry")
async def telemetry_websocket(websocket: WebSocket):
    """
    WebSocket for streaming real-time telemetry:
    - Laser power, scan frequency, frame rate
    - Bubble density, resonant frequency, cavitation threshold
    - Temperature, system health
    """
    await websocket.accept()
    logger.info("Telemetry WebSocket connected")

    try:
        while True:
            # Send telemetry every 100ms
            if all([bubble_gen, laser_ctrl, renderer]):
                telemetry = {
                    "laser": laser_ctrl.get_telemetry(),
                    "bubbles": bubble_gen.get_telemetry(),
                    "render": renderer.get_telemetry(),
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
