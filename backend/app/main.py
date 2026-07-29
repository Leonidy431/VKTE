"""
FastAPI application for Volumetric Display control.

References:
- VOLUMETRIC_DISPLAY_SPECIFICATION.md
- docs/BUBBLE_TECHNOLOGY.md
- docs/HARDWARE_BOM.md
- ARCHITECTURE_AUDIT.md (security & validation)
"""

from fastapi import FastAPI, HTTPException, WebSocket, Depends, status
from fastapi.middleware.cors import CORSMiddleware
from fastapi.security import HTTPBearer, HTTPAuthCredentials
from slowapi import Limiter
from slowapi.util import get_remote_address
from contextlib import asynccontextmanager
import logging
import asyncio
import os
from typing import Optional
import structlog

from app.modules.volumetric.bubble_generator import BubbleGenerator
from app.modules.volumetric.laser_controller import LaserController
from app.modules.volumetric.volumetric_renderer import VolumetricRenderer
from app.modules.volumetric.hud_renderer import HUDRenderer, HUDMode, TelemetryFrame
from app.config import Settings
from app.schemas import (
    HUDRenderRequest, HUDModeRequest, HUDBrightnessRequest,
    RenderObjectRequest, BubbleStartRequest, HealthResponse
)

# Setup structured logging
structlog.configure(
    processors=[
        structlog.stdlib.filter_by_level,
        structlog.stdlib.add_logger_name,
        structlog.stdlib.add_log_level,
        structlog.stdlib.PositionalArgumentsFormatter(),
        structlog.processors.TimeStamper(fmt="iso"),
        structlog.processors.StackInfoRenderer(),
        structlog.processors.format_exc_info,
        structlog.processors.UnicodeDecoder(),
        structlog.processors.JSONRenderer()
    ],
    context_class=dict,
    logger_factory=structlog.stdlib.LoggerFactory(),
    cache_logger_on_first_use=True,
)

logger = structlog.get_logger(__name__)
settings = Settings()

# Rate limiting (prevent DDoS)
limiter = Limiter(key_func=get_remote_address)

# API key authentication (from environment or hardcoded for demo)
VALID_API_KEYS = {settings.get("API_KEY", "demo-key-change-in-production")}

security = HTTPBearer()


def verify_api_key(credentials: HTTPAuthCredentials = Depends(security)) -> str:
    """Verify API key from Authorization header."""
    if credentials.credentials not in VALID_API_KEYS:
        logger.warning("invalid_api_key", key=credentials.credentials[:10])
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Invalid or missing API key",
            headers={"WWW-Authenticate": "Bearer"},
        )
    return credentials.credentials


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
    version="0.2.0",  # Updated with audit fixes
    lifespan=lifespan,
)

# Add rate limiter
app.state.limiter = limiter
app.add_exception_handler(limiter.RateLimitExceeded, lambda r, e: HTTPException(
    status_code=429,
    detail="Rate limit exceeded"
))

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
@limiter.limit("100/minute")
async def health_check(request):
    """Basic health check endpoint (no auth required)."""
    logger.info("health_check")
    return HealthResponse()


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
@limiter.limit("20/minute")
async def start_bubbles(
    request,
    req: BubbleStartRequest,
    token: str = Depends(verify_api_key),
):
    """Start bubble generation at specified frequency and duty cycle.

    Requires: Authorization: Bearer <API_KEY>
    """
    if not bubble_gen:
        logger.error("bubble_gen_not_initialized")
        raise HTTPException(status_code=503, detail="Bubble generator not initialized")

    try:
        bubble_gen.set_frequency(req.frequency_hz)
        bubble_gen.set_duty_cycle(req.duty_cycle)
        bubble_gen.start()
        logger.info("bubbles_started", frequency_hz=req.frequency_hz, duty_cycle=req.duty_cycle)
        return {
            "status": "bubbles_started",
            "frequency_hz": req.frequency_hz,
            "duty_cycle": req.duty_cycle
        }
    except ValueError as e:
        logger.warning("bubble_start_error", error=str(e))
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
@limiter.limit("60/minute")
async def set_laser_power(
    request,
    req: LaserPowerRequest,
    token: str = Depends(verify_api_key),
):
    """Set laser output power in watts.

    Requires: Authorization: Bearer <API_KEY>
    """
    if not laser_ctrl:
        logger.error("laser_ctrl_not_initialized")
        raise HTTPException(status_code=503, detail="Laser controller not initialized")

    try:
        laser_ctrl.set_power(req.power_w)
        logger.info("laser_power_set", power_w=req.power_w)
        return {"status": "laser_power_set", "power_w": req.power_w}
    except ValueError as e:
        logger.warning("laser_power_error", error=str(e))
        raise HTTPException(status_code=400, detail=str(e))


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
@limiter.limit("30/minute")
async def render_3d_object(
    request,
    req: RenderObjectRequest,
    token: str = Depends(verify_api_key),
):
    """
    Render a 3D object type in the volumetric display.

    Supported types: sphere, cube, torus, mesh_custom

    Requires: Authorization: Bearer <API_KEY>
    """
    if not renderer:
        logger.error("renderer_not_initialized")
        raise HTTPException(status_code=503, detail="Renderer not initialized")

    try:
        renderer.render_object(req.object_type, req.scale)
        logger.info("render_object_started", object_type=req.object_type, scale=req.scale)
        return {"status": "render_started", "object": req.object_type, "scale": req.scale}
    except ValueError as e:
        logger.warning("render_error", error=str(e))
        raise HTTPException(status_code=400, detail=str(e))


# ============================================================================
# HUD Control Endpoints
# ============================================================================

@app.post("/api/v1/hud/render")
@limiter.limit("30/minute")  # 30 renders per minute max
async def render_hud_frame(
    request,
    req: HUDRenderRequest,
    token: str = Depends(verify_api_key),
):
    """
    Render HUD frame with telemetry data.

    Supports both Land (Volga automotive) and Marine (underwater) modes.
    Returns metadata about the rendered frame.

    Requires: Authorization: Bearer <API_KEY>
    """
    if not hud_renderer:
        logger.error("hud_renderer_not_initialized")
        raise HTTPException(status_code=503, detail="HUD renderer not initialized")

    try:
        telemetry = TelemetryFrame(
            speed_kmh=req.speed_kmh,
            engine_temp_c=req.engine_temp_c,
            battery_voltage_v=req.battery_voltage_v,
            engine_current_a=req.engine_current_a,
            depth_m=req.depth_m,
            water_temp_c=req.water_temp_c,
            pressure_bar=req.pressure_bar,
            salinity_ppt=req.salinity_ppt,
            mode=hud_renderer.mode,
        )

        result = hud_renderer.render_frame(telemetry)
        logger.info("hud_render_success", mode=hud_renderer.mode.value, frame=result.get("frame"))
        return result
    except ValueError as e:
        logger.warning("hud_render_validation_error", error=str(e))
        raise HTTPException(status_code=400, detail=f"Validation error: {str(e)}")
    except Exception as e:
        logger.error("hud_render_error", error=str(e), exc_info=True)
        raise HTTPException(status_code=500, detail="Internal render error")


@app.post("/api/v1/hud/mode")
@limiter.limit("60/minute")
async def set_hud_mode(
    request,
    req: HUDModeRequest,
    token: str = Depends(verify_api_key),
):
    """
    Set HUD operating mode: land, marine, debug, off

    - land: Volga 2410 automotive display (speedometer, engine status, battery)
    - marine: Underwater display (depth gauge, water temp, pressure, salinity)
    - debug: Simulator display (all telemetry at once)
    - off: Display off

    Requires: Authorization: Bearer <API_KEY>
    """
    if not hud_renderer:
        logger.error("hud_renderer_not_initialized")
        raise HTTPException(status_code=503, detail="HUD renderer not initialized")

    try:
        hud_mode = HUDMode(req.mode)
        hud_renderer.set_mode(hud_mode)
        logger.info("hud_mode_changed", mode=hud_mode.value)
        return {"status": "hud_mode_set", "mode": hud_mode.value}
    except ValueError as e:
        logger.warning("hud_mode_invalid", mode=req.mode, error=str(e))
        raise HTTPException(status_code=400, detail=str(e))


@app.post("/api/v1/hud/brightness")
@limiter.limit("60/minute")
async def set_hud_brightness(
    request,
    req: HUDBrightnessRequest,
    token: str = Depends(verify_api_key),
):
    """Set HUD display brightness (0-100%).

    Requires: Authorization: Bearer <API_KEY>
    """
    if not hud_renderer:
        logger.error("hud_renderer_not_initialized")
        raise HTTPException(status_code=503, detail="HUD renderer not initialized")

    try:
        hud_renderer.set_brightness(req.percent)
        brightness_cd_m2 = int(4000 * req.percent / 100)
        logger.info("hud_brightness_set", percent=req.percent, cd_m2=brightness_cd_m2)
        return {
            "status": "hud_brightness_set",
            "brightness_percent": req.percent,
            "brightness_cd_m2": brightness_cd_m2,  # DLP TRP-4500 spec: 4000 cd/m²
        }
    except ValueError as e:
        logger.warning("hud_brightness_error", error=str(e))
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
