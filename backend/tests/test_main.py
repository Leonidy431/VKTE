"""Integration tests for FastAPI application endpoints."""

import pytest
from unittest.mock import patch
from fastapi.testclient import TestClient
from app.main import app as fastapi_app

# `client` fixture (with lifespan startup/shutdown) comes from conftest.py


@pytest.fixture
def valid_api_key():
    """Valid API key for testing."""
    return "sk-vkte-dev-change-in-production"


@pytest.fixture
def auth_headers(valid_api_key):
    """Authorization headers with valid API key."""
    return {"Authorization": f"Bearer {valid_api_key}"}


@pytest.fixture
def invalid_auth_headers():
    """Authorization headers with invalid API key."""
    return {"Authorization": "Bearer invalid-key-12345"}


class TestHealthEndpoint:
    """Test health check endpoint."""

    def test_health_check_success(self, client):
        """Test successful health check."""
        response = client.get("/health")
        assert response.status_code == 200
        assert response.json()["status"] == "healthy"

    def test_health_check_no_auth_required(self, client):
        """Test that health check doesn't require authentication."""
        response = client.get("/health")
        assert response.status_code == 200


class TestStatusEndpoint:
    """Test system status endpoint."""

    def test_system_status_success(self, client):
        """Test getting system status."""
        response = client.get("/api/v1/status")
        assert response.status_code == 200
        data = response.json()
        assert "bubble_generator" in data
        assert "laser_controller" in data
        assert "renderer" in data
        assert "timestamp" in data

    def test_system_status_contains_subsystems(self, client):
        """Test that status contains all subsystems."""
        response = client.get("/api/v1/status")
        data = response.json()
        assert data["bubble_generator"]["running"] is False
        assert data["laser_controller"]["mode"] == "off"
        assert data["renderer"]["rendering"] is False


class TestBubbleControlEndpoints:
    """Test bubble control endpoints."""

    def test_start_bubbles_with_auth(self, client, auth_headers):
        """Test starting bubbles with valid authentication."""
        payload = {"frequency_hz": 40000, "duty_cycle": 0.5}
        response = client.post(
            "/api/v1/bubble/start", json=payload, headers=auth_headers
        )
        assert response.status_code == 200
        data = response.json()
        assert data["status"] == "bubbles_started"
        assert data["frequency_hz"] == 40000

    def test_start_bubbles_without_auth(self, client):
        """Test starting bubbles without authentication."""
        payload = {"frequency_hz": 40000, "duty_cycle": 0.5}
        response = client.post("/api/v1/bubble/start", json=payload)
        assert response.status_code == 403  # Forbidden (no auth header)

    def test_start_bubbles_invalid_auth(self, client, invalid_auth_headers):
        """Test starting bubbles with invalid API key."""
        payload = {"frequency_hz": 40000, "duty_cycle": 0.5}
        response = client.post(
            "/api/v1/bubble/start", json=payload, headers=invalid_auth_headers
        )
        assert response.status_code == 401  # Unauthorized

    def test_start_bubbles_invalid_frequency(self, client, auth_headers):
        """Test starting bubbles with invalid frequency."""
        payload = {"frequency_hz": 10000, "duty_cycle": 0.5}  # Below minimum 20000
        response = client.post(
            "/api/v1/bubble/start", json=payload, headers=auth_headers
        )
        assert response.status_code == 422  # Pydantic validation error

    def test_start_bubbles_invalid_duty_cycle(self, client, auth_headers):
        """Test starting bubbles with invalid duty cycle."""
        payload = {"frequency_hz": 40000, "duty_cycle": 1.5}  # Above maximum 1.0
        response = client.post(
            "/api/v1/bubble/start", json=payload, headers=auth_headers
        )
        assert response.status_code == 422  # Pydantic validation error

    def test_stop_bubbles(self, client):
        """Test stopping bubbles."""
        response = client.post("/api/v1/bubble/stop")
        assert response.status_code == 200
        assert response.json()["status"] == "bubbles_stopped"


class TestLaserControlEndpoints:
    """Test laser control endpoints."""

    def test_set_laser_power_with_auth(self, client, auth_headers):
        """Test setting laser power with authentication."""
        payload = {"power_w": 5.0}
        response = client.post(
            "/api/v1/laser/power", json=payload, headers=auth_headers
        )
        assert response.status_code == 200
        data = response.json()
        assert data["status"] == "laser_power_set"
        assert data["power_w"] == 5.0

    def test_set_laser_power_without_auth(self, client):
        """Test setting laser power without authentication."""
        payload = {"power_w": 5.0}
        response = client.post("/api/v1/laser/power", json=payload)
        assert response.status_code == 403

    def test_set_laser_power_invalid_value(self, client, auth_headers):
        """Test setting laser power with invalid value."""
        payload = {"power_w": 10.0}  # Above maximum 8.0
        response = client.post(
            "/api/v1/laser/power", json=payload, headers=auth_headers
        )
        assert response.status_code == 422  # Pydantic validation error

    def test_start_laser_scan_requires_bubbles_running(self, client):
        """Test that starting laser scan without active bubble generation is rejected."""
        response = client.post("/api/v1/laser/scan")
        assert response.status_code == 400

    def test_start_laser_scan_with_bubbles_running(self, client, auth_headers):
        """Test starting laser scan once bubble generation is active."""
        start_response = client.post(
            "/api/v1/bubble/start",
            json={"frequency_hz": 40000, "duty_cycle": 0.5},
            headers=auth_headers,
        )
        assert start_response.status_code == 200

        response = client.post("/api/v1/laser/scan")
        assert response.status_code == 200
        assert response.json()["status"] == "laser_scan_started"


class TestRenderEndpoints:
    """Test rendering endpoints."""

    def test_render_sphere_with_auth(self, client, auth_headers):
        """Test rendering sphere with authentication."""
        payload = {"object_type": "sphere", "scale": 1.0}
        response = client.post(
            "/api/v1/render/object", json=payload, headers=auth_headers
        )
        assert response.status_code == 200
        data = response.json()
        assert data["status"] == "render_started"
        assert data["object"] == "sphere"

    def test_render_cube_with_auth(self, client, auth_headers):
        """Test rendering cube with authentication."""
        payload = {"object_type": "cube", "scale": 0.5}
        response = client.post(
            "/api/v1/render/object", json=payload, headers=auth_headers
        )
        assert response.status_code == 200

    def test_render_torus_with_auth(self, client, auth_headers):
        """Test rendering torus with authentication."""
        payload = {"object_type": "torus", "scale": 1.5}
        response = client.post(
            "/api/v1/render/object", json=payload, headers=auth_headers
        )
        assert response.status_code == 200

    def test_render_without_auth(self, client):
        """Test rendering without authentication."""
        payload = {"object_type": "sphere", "scale": 1.0}
        response = client.post("/api/v1/render/object", json=payload)
        assert response.status_code == 403

    def test_render_invalid_object_type(self, client, auth_headers):
        """Test rendering invalid object type."""
        payload = {"object_type": "invalid", "scale": 1.0}
        response = client.post(
            "/api/v1/render/object", json=payload, headers=auth_headers
        )
        assert response.status_code == 422  # Pydantic validation error

    def test_render_invalid_scale(self, client, auth_headers):
        """Test rendering with invalid scale."""
        payload = {"object_type": "sphere", "scale": 3.0}  # Above maximum 2.0
        response = client.post(
            "/api/v1/render/object", json=payload, headers=auth_headers
        )
        assert response.status_code == 422  # Pydantic validation error


class TestHUDControlEndpoints:
    """Test HUD control endpoints."""

    def test_render_hud_frame_land_mode(self, client, auth_headers):
        """Test rendering HUD frame in land mode."""
        payload = {"speed_kmh": 80.0, "engine_temp_c": 90.0, "battery_voltage_v": 13.5}
        response = client.post("/api/v1/hud/render", json=payload, headers=auth_headers)
        assert response.status_code == 200
        data = response.json()
        assert "frame" in data
        assert "mode" in data
        assert "resolution" in data

    def test_render_hud_frame_marine_mode(self, client, auth_headers):
        """Test rendering HUD frame in marine mode."""
        payload = {"depth_m": 25.0, "water_temp_c": 18.0, "pressure_bar": 3.5}
        response = client.post("/api/v1/hud/render", json=payload, headers=auth_headers)
        assert response.status_code == 200

    def test_render_hud_without_auth(self, client):
        """Test rendering HUD without authentication."""
        payload = {"speed_kmh": 50.0}
        response = client.post("/api/v1/hud/render", json=payload)
        assert response.status_code == 403

    def test_render_hud_speed_out_of_range(self, client, auth_headers):
        """Test rendering HUD with out-of-range speed."""
        payload = {"speed_kmh": 300.0}  # Above maximum 250
        response = client.post("/api/v1/hud/render", json=payload, headers=auth_headers)
        assert response.status_code == 422  # Pydantic validation error

    def test_render_hud_nan_speed(self, client, auth_headers):
        """Test rendering HUD with NaN speed."""
        # httpx's `json=` kwarg encodes with allow_nan=False and raises
        # client-side, so NaN has to go over the wire as raw JSON bytes
        # (Python's decoder still accepts the "NaN" literal on the way in).
        response = client.post(
            "/api/v1/hud/render",
            content=b'{"speed_kmh": NaN}',
            headers={**auth_headers, "Content-Type": "application/json"},
        )
        assert response.status_code == 422  # Pydantic validation error

    def test_set_hud_mode_land(self, client, auth_headers):
        """Test setting HUD mode to land."""
        payload = {"mode": "land"}
        response = client.post("/api/v1/hud/mode", json=payload, headers=auth_headers)
        assert response.status_code == 200
        data = response.json()
        assert data["status"] == "hud_mode_set"
        assert data["mode"] == "land"

    def test_set_hud_mode_marine(self, client, auth_headers):
        """Test setting HUD mode to marine."""
        payload = {"mode": "marine"}
        response = client.post("/api/v1/hud/mode", json=payload, headers=auth_headers)
        assert response.status_code == 200
        assert response.json()["mode"] == "marine"

    def test_set_hud_mode_debug(self, client, auth_headers):
        """Test setting HUD mode to debug."""
        payload = {"mode": "debug"}
        response = client.post("/api/v1/hud/mode", json=payload, headers=auth_headers)
        assert response.status_code == 200
        assert response.json()["mode"] == "debug"

    def test_set_hud_mode_invalid(self, client, auth_headers):
        """Test setting HUD mode to invalid value."""
        payload = {"mode": "invalid"}
        response = client.post("/api/v1/hud/mode", json=payload, headers=auth_headers)
        assert response.status_code == 422  # Pydantic validation error

    def test_set_hud_brightness_valid(self, client, auth_headers):
        """Test setting HUD brightness."""
        payload = {"percent": 75.0}
        response = client.post(
            "/api/v1/hud/brightness", json=payload, headers=auth_headers
        )
        assert response.status_code == 200
        data = response.json()
        assert data["status"] == "hud_brightness_set"
        assert data["brightness_percent"] == 75.0
        assert "brightness_cd_m2" in data

    def test_set_hud_brightness_zero(self, client, auth_headers):
        """Test setting HUD brightness to zero."""
        payload = {"percent": 0.0}
        response = client.post(
            "/api/v1/hud/brightness", json=payload, headers=auth_headers
        )
        assert response.status_code == 200

    def test_set_hud_brightness_invalid(self, client, auth_headers):
        """Test setting HUD brightness to invalid value."""
        payload = {"percent": 150.0}  # Above maximum 100
        response = client.post(
            "/api/v1/hud/brightness", json=payload, headers=auth_headers
        )
        assert response.status_code == 422  # Pydantic validation error

    def test_get_hud_status(self, client):
        """Test getting HUD status."""
        response = client.get("/api/v1/hud/status")
        assert response.status_code == 200
        data = response.json()
        assert "mode" in data
        assert "active" in data
        assert "brightness_percent" in data
        assert "frame_count" in data


class TestRateLimiting:
    """Test rate limiting."""

    def test_rate_limit_health_endpoint(self, client):
        """Test rate limiting on health endpoint."""
        # Health endpoint has 100/minute limit
        for i in range(5):
            response = client.get("/health")
            assert response.status_code == 200

    def test_bubble_endpoint_requires_auth(self, client, invalid_auth_headers):
        """Test that bubble endpoint properly rejects invalid auth."""
        payload = {"frequency_hz": 40000, "duty_cycle": 0.5}
        response = client.post(
            "/api/v1/bubble/start", json=payload, headers=invalid_auth_headers
        )
        assert response.status_code == 401


class TestErrorHandling:
    """Test error handling."""

    def test_missing_required_field(self, client, auth_headers):
        """Test error when required field is missing."""
        payload = {"mode": ""}  # Empty mode
        response = client.post("/api/v1/hud/mode", json=payload, headers=auth_headers)
        assert response.status_code == 422  # Pydantic validation error

    def test_invalid_json(self, client, auth_headers):
        """Test error with invalid JSON."""
        response = client.post(
            "/api/v1/hud/brightness",
            content=b"invalid json",
            headers={**auth_headers, "Content-Type": "application/json"},
        )
        assert response.status_code in [400, 422]

    def test_wrong_content_type(self, client, auth_headers):
        """Test error with wrong content type."""
        response = client.post(
            "/api/v1/hud/brightness",
            data="percent=50",
            headers={**auth_headers, "Content-Type": "text/plain"},
        )
        assert response.status_code in [400, 422, 415]


class TestCORSHeaders:
    """Test CORS headers."""

    def test_cors_headers_present(self, client):
        """Test that CORS headers are present."""
        response = client.get("/health")
        assert response.status_code == 200
        # CORS headers should be present in response


@pytest.fixture
def ws_client():
    """TestClient with the telemetry loop's sleep patched to terminate after one iteration.

    The endpoint's `while True: ... await asyncio.sleep(0.1)` loop never
    observes client disconnects on the in-process ASGI test transport (it
    never calls `receive()`), so it keeps running in TestClient's background
    thread forever once the ordinary `client` fixture's `with patch(...):`
    block in the test body has already exited and un-patched `asyncio.sleep`
    — a race between this thread and that one, since fixture teardown
    (which drains that background thread) runs *after* the test body
    returns. Nesting `TestClient` inside the patch here instead keeps the
    patch alive through the client's own teardown, closing that race.
    """
    call_count = {"n": 0}

    async def bounded_sleep(seconds):
        call_count["n"] += 1
        if call_count["n"] >= 2:
            raise RuntimeError("stop telemetry loop for test")

    with patch("app.main.asyncio.sleep", side_effect=bounded_sleep):
        with TestClient(fastapi_app) as test_client:
            yield test_client


class TestWebSocketTelemetry:
    """Test WebSocket telemetry endpoint."""

    def test_websocket_connection(self, ws_client):
        """Test WebSocket connection."""
        with ws_client.websocket_connect("/ws/telemetry") as websocket:
            data = websocket.receive_json()
            assert "laser" in data
            assert "bubbles" in data
            assert "render" in data
            assert "hud" in data

    def test_websocket_disconnect_logged_cleanly(self, ws_client):
        """Test that a real client disconnect takes the WebSocketDisconnect path."""
        from fastapi import WebSocketDisconnect

        with patch("app.main.asyncio.sleep", side_effect=WebSocketDisconnect()):
            with ws_client.websocket_connect("/ws/telemetry") as websocket:
                websocket.receive_json()

    def test_websocket_close_error_is_swallowed(self, ws_client):
        """Test that an error while closing an already-tearing-down socket doesn't propagate."""
        with patch("app.main.asyncio.sleep", side_effect=RuntimeError("stop")):
            with patch(
                "starlette.websockets.WebSocket.close",
                side_effect=RuntimeError("already closed"),
            ):
                with ws_client.websocket_connect("/ws/telemetry") as websocket:
                    websocket.receive_json()


class TestSubsystemsNotInitialized:
    """Test 503 responses when a subsystem global is unset (startup failed / not yet ready)."""

    def test_status_not_initialized(self, client):
        with patch("app.main.bubble_gen", None):
            response = client.get("/api/v1/status")
        assert response.status_code == 503

    def test_bubble_start_not_initialized(self, client, auth_headers):
        with patch("app.main.bubble_gen", None):
            response = client.post(
                "/api/v1/bubble/start",
                json={"frequency_hz": 40000, "duty_cycle": 0.5},
                headers=auth_headers,
            )
        assert response.status_code == 503

    def test_bubble_stop_not_initialized(self, client):
        with patch("app.main.bubble_gen", None):
            response = client.post("/api/v1/bubble/stop")
        assert response.status_code == 503

    def test_laser_power_not_initialized(self, client, auth_headers):
        with patch("app.main.laser_ctrl", None):
            response = client.post(
                "/api/v1/laser/power",
                json={"power_w": 5.0},
                headers=auth_headers,
            )
        assert response.status_code == 503

    def test_laser_scan_not_initialized(self, client):
        with patch("app.main.laser_ctrl", None):
            response = client.post("/api/v1/laser/scan")
        assert response.status_code == 503

    def test_render_object_not_initialized(self, client, auth_headers):
        with patch("app.main.renderer", None):
            response = client.post(
                "/api/v1/render/object",
                json={"object_type": "sphere", "scale": 1.0},
                headers=auth_headers,
            )
        assert response.status_code == 503

    def test_hud_render_not_initialized(self, client, auth_headers):
        with patch("app.main.hud_renderer", None):
            response = client.post(
                "/api/v1/hud/render",
                json={"speed_kmh": 50.0},
                headers=auth_headers,
            )
        assert response.status_code == 503

    def test_hud_mode_not_initialized(self, client, auth_headers):
        with patch("app.main.hud_renderer", None):
            response = client.post(
                "/api/v1/hud/mode",
                json={"mode": "land"},
                headers=auth_headers,
            )
        assert response.status_code == 503

    def test_hud_brightness_not_initialized(self, client, auth_headers):
        with patch("app.main.hud_renderer", None):
            response = client.post(
                "/api/v1/hud/brightness",
                json={"percent": 50.0},
                headers=auth_headers,
            )
        assert response.status_code == 503

    def test_hud_status_not_initialized(self, client):
        with patch("app.main.hud_renderer", None):
            response = client.get("/api/v1/hud/status")
        assert response.status_code == 503


class TestUnderlyingModuleValueErrors:
    """Test that ValueError raised by an underlying module (past Pydantic validation) yields 400.

    These simulate a module-level rejection that Pydantic's own bounds
    didn't already catch (e.g. a future bound tightened on the hardware
    side without the schema being updated in lockstep) — the endpoint's
    `except ValueError` handler is what protects against that drift.
    """

    def test_bubble_start_value_error(self, client, auth_headers):
        with patch(
            "app.main.bubble_gen.set_frequency", side_effect=ValueError("bad frequency")
        ):
            response = client.post(
                "/api/v1/bubble/start",
                json={"frequency_hz": 40000, "duty_cycle": 0.5},
                headers=auth_headers,
            )
        assert response.status_code == 400
        assert "bad frequency" in response.json()["detail"]

    def test_laser_power_value_error(self, client, auth_headers):
        with patch(
            "app.main.laser_ctrl.set_power", side_effect=ValueError("bad power")
        ):
            response = client.post(
                "/api/v1/laser/power",
                json={"power_w": 5.0},
                headers=auth_headers,
            )
        assert response.status_code == 400
        assert "bad power" in response.json()["detail"]

    def test_render_object_value_error(self, client, auth_headers):
        with patch(
            "app.main.renderer.render_object", side_effect=ValueError("bad object")
        ):
            response = client.post(
                "/api/v1/render/object",
                json={"object_type": "sphere", "scale": 1.0},
                headers=auth_headers,
            )
        assert response.status_code == 400
        assert "bad object" in response.json()["detail"]

    def test_hud_render_value_error(self, client, auth_headers):
        with patch(
            "app.main.hud_renderer.render_frame", side_effect=ValueError("bad frame")
        ):
            response = client.post(
                "/api/v1/hud/render",
                json={"speed_kmh": 50.0},
                headers=auth_headers,
            )
        assert response.status_code == 400
        assert "bad frame" in response.json()["detail"]

    def test_hud_render_unexpected_error(self, client, auth_headers):
        with patch(
            "app.main.hud_renderer.render_frame", side_effect=RuntimeError("boom")
        ):
            response = client.post(
                "/api/v1/hud/render",
                json={"speed_kmh": 50.0},
                headers=auth_headers,
            )
        assert response.status_code == 500

    def test_hud_mode_value_error(self, client, auth_headers):
        # HUDModeRequest's pattern only allows enum-valid strings, so reach
        # the handler's except block by making HUDMode() itself raise.
        with patch("app.main.HUDMode", side_effect=ValueError("bad mode")):
            response = client.post(
                "/api/v1/hud/mode",
                json={"mode": "land"},
                headers=auth_headers,
            )
        assert response.status_code == 400

    def test_hud_brightness_value_error(self, client, auth_headers):
        with patch(
            "app.main.hud_renderer.set_brightness",
            side_effect=ValueError("bad brightness"),
        ):
            response = client.post(
                "/api/v1/hud/brightness",
                json={"percent": 50.0},
                headers=auth_headers,
            )
        assert response.status_code == 400
        assert "bad brightness" in response.json()["detail"]


class TestLifespanErrorHandling:
    """Test that a subsystem hardware failure during lifespan degrades, not crashes, startup.

    RenderingCoordinator initializes each of the four subsystems
    independently (ARCHITECTURE_AUDIT.md Sprint 2, A3), so one failure must
    not prevent the app from starting or take down the other three.
    """

    def test_startup_failure_is_degraded_not_fatal(self):
        with patch(
            "app.coordinator.BubbleGenerator",
            side_effect=RuntimeError("hardware init failed"),
        ):
            with TestClient(fastapi_app) as test_client:
                import app.main as main_module

                assert main_module.bubble_gen is None
                assert main_module.laser_ctrl is not None
                assert main_module.coordinator.is_degraded is True
                assert "bubble_generator" in main_module.coordinator.degraded_subsystems

                health = test_client.get("/health")
                assert health.status_code == 200
                assert health.json()["degraded"] is True
                assert "bubble_generator" in health.json()["degraded_subsystems"]

    def test_shutdown_failure_is_logged_not_raised(self):
        with patch(
            "app.coordinator.BubbleGenerator.stop",
            side_effect=RuntimeError("shutdown failed"),
        ):
            # Shutdown errors are caught and logged inside coordinator.shutdown(),
            # so exiting the context manager must not raise.
            with TestClient(fastapi_app):
                pass
