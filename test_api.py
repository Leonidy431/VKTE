#!/usr/bin/env python3
"""
Quick API test script for Volumetric Display backend.

Run after: docker compose up --build backend
"""

import httpx
import json
import asyncio
import sys
from typing import Optional

BASE_URL = "http://localhost:8000"
TIMEOUT = 5.0


async def test_health():
    """Test health endpoint."""
    print("🏥 Health Check...")
    try:
        async with httpx.AsyncClient(timeout=TIMEOUT) as client:
            response = await client.get(f"{BASE_URL}/health")
            if response.status_code == 200:
                print(f"  ✅ Status: {response.json()['status']}")
                return True
            else:
                print(f"  ❌ Status {response.status_code}")
                return False
    except Exception as e:
        print(f"  ❌ Error: {e}")
        return False


async def test_system_status():
    """Get full system status."""
    print("\n📊 System Status...")
    try:
        async with httpx.AsyncClient(timeout=TIMEOUT) as client:
            response = await client.get(f"{BASE_URL}/api/v1/status")
            if response.status_code == 200:
                data = response.json()
                print(json.dumps(data, indent=2))
                return True
            else:
                print(f"  ❌ Status {response.status_code}")
                return False
    except Exception as e:
        print(f"  ❌ Error: {e}")
        return False


async def test_bubble_start():
    """Start bubble generation."""
    print("\n🫧 Starting Bubble Generation...")
    try:
        async with httpx.AsyncClient(timeout=TIMEOUT) as client:
            response = await client.post(
                f"{BASE_URL}/api/v1/bubble/start",
                params={"frequency_hz": 40000, "duty_cycle": 0.5}
            )
            if response.status_code == 200:
                print(f"  ✅ {response.json()['status']}")
                return True
            else:
                print(f"  ❌ Status {response.status_code}: {response.text}")
                return False
    except Exception as e:
        print(f"  ❌ Error: {e}")
        return False


async def test_laser_power():
    """Set laser power."""
    print("\n🔴 Setting Laser Power...")
    try:
        async with httpx.AsyncClient(timeout=TIMEOUT) as client:
            response = await client.post(
                f"{BASE_URL}/api/v1/laser/power",
                params={"power_w": 5.0}
            )
            if response.status_code == 200:
                print(f"  ✅ {response.json()['status']}: {response.json()['power_w']} W")
                return True
            else:
                print(f"  ❌ Status {response.status_code}: {response.text}")
                return False
    except Exception as e:
        print(f"  ❌ Error: {e}")
        return False


async def test_render_object():
    """Render 3D object."""
    print("\n🎨 Rendering 3D Object (sphere)...")
    try:
        async with httpx.AsyncClient(timeout=TIMEOUT) as client:
            response = await client.post(
                f"{BASE_URL}/api/v1/render/object",
                params={"object_type": "sphere", "scale": 1.0}
            )
            if response.status_code == 200:
                print(f"  ✅ {response.json()['status']}")
                return True
            else:
                print(f"  ❌ Status {response.status_code}: {response.text}")
                return False
    except Exception as e:
        print(f"  ❌ Error: {e}")
        return False


async def test_laser_scan():
    """Start laser scanning."""
    print("\n📍 Starting Laser Scan...")
    try:
        async with httpx.AsyncClient(timeout=TIMEOUT) as client:
            response = await client.post(f"{BASE_URL}/api/v1/laser/scan")
            if response.status_code == 200:
                print(f"  ✅ {response.json()['status']}")
                return True
            else:
                print(f"  ❌ Status {response.status_code}: {response.text}")
                return False
    except Exception as e:
        print(f"  ❌ Error: {e}")
        return False


async def main():
    """Run all tests."""
    print("=" * 60)
    print("Volumetric Display API Test Suite")
    print("=" * 60)
    print(f"\nTarget: {BASE_URL}\n")

    results = []

    # Test connectivity
    if not await test_health():
        print("\n❌ Cannot connect to backend. Is it running?")
        print(f"   docker compose up --build backend -d")
        sys.exit(1)

    results.append(("Health", await test_health()))
    results.append(("Status", await test_system_status()))
    results.append(("Bubble Start", await test_bubble_start()))
    results.append(("Laser Power", await test_laser_power()))
    results.append(("Render Object", await test_render_object()))
    results.append(("Laser Scan", await test_laser_scan()))

    # Summary
    print("\n" + "=" * 60)
    print("Test Summary")
    print("=" * 60)
    for name, result in results:
        status = "✅" if result else "❌"
        print(f"{status} {name}")

    passed = sum(1 for _, r in results if r)
    total = len(results)
    print(f"\nResult: {passed}/{total} tests passed")

    if passed == total:
        print("\n🎉 All tests passed!")
        sys.exit(0)
    else:
        print(f"\n⚠️  {total - passed} test(s) failed")
        sys.exit(1)


if __name__ == "__main__":
    asyncio.run(main())
