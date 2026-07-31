"""Pytest configuration and fixtures."""

import pytest
from fastapi.testclient import TestClient
from app.main import app as fastapi_app


@pytest.fixture
def client():
    """FastAPI test client with lifespan (startup/shutdown) triggered."""
    with TestClient(fastapi_app) as test_client:
        yield test_client


@pytest.fixture
def api_key():
    """Valid API key for testing."""
    return "sk-vkte-dev-change-in-production"


@pytest.fixture
def auth_headers(api_key):
    """Authorization headers with API key."""
    return {"Authorization": f"Bearer {api_key}"}
