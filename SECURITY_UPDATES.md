# Security Updates (Phase 5.5 Audit Implementation)

**Date:** 2026-07-29  
**Phase:** 5.5 (Prototype + Security Hardening)

---

## What Changed

### 1. Input Validation (Pydantic Schemas)
All API parameters are now validated with:
- Type checking
- Range bounds (speed_kmh: 0-250, depth_m: 0-500, etc.)
- NaN/Inf rejection
- Regex patterns (mode must be land/marine/debug/off)

**Example before:**
```bash
curl -X POST "http://localhost:8000/api/v1/hud/render?speed_kmh=99999999"
# No validation → potential crash
```

**Example after:**
```bash
curl -X POST "http://localhost:8000/api/v1/hud/render" \
  -H "Authorization: Bearer sk-vkte-dev-change-in-production" \
  -H "Content-Type: application/json" \
  -d '{
    "speed_kmh": 60.0,
    "engine_temp_c": 85.0,
    "battery_voltage_v": 12.5
  }'
```

### 2. API Key Authentication
All POST endpoints now require authentication:

```bash
# Export your API key
export API_KEY="sk-vkte-dev-change-in-production"  # Change in .env!

# Or pass in header
curl -X POST "http://localhost:8000/api/v1/hud/brightness" \
  -H "Authorization: Bearer $API_KEY" \
  -H "Content-Type: application/json" \
  -d '{"percent": 75}'
```

**Health check** (no auth required):
```bash
curl http://localhost:8000/health
```

### 3. Rate Limiting
Endpoints are now rate-limited per source IP:

| Endpoint | Limit | Reason |
|----------|-------|--------|
| /health | 100/min | Monitoring |
| /api/v1/hud/render | 30/min | Expensive operation |
| /api/v1/hud/brightness | 60/min | Quick operation |
| /api/v1/laser/power | 60/min | Safe operation |
| /api/v1/bubble/start | 20/min | Hardware safety |
| /api/v1/render/object | 30/min | Expensive operation |

**Error 429** = Rate limit exceeded. Wait & retry.

### 4. Structured JSON Logging
All logs are now JSON-formatted with structured fields:

**Before:**
```
INFO:app.main:HUDRenderer initialized (mode=land, brightness=100.0%)
```

**After:**
```json
{
  "event": "hud_render_success",
  "mode": "land",
  "frame": 42,
  "timestamp": "2026-07-29T10:30:45.123Z",
  "level": "info"
}
```

Benefits:
- Parseable by log aggregation systems (ELK, Datadog, etc.)
- Better debugging with structured context
- Correlation IDs for request tracing (Phase 9)

### 5. Environment Variable Configuration

Create `.env` file in `backend/` directory:

```bash
# .env
HOST=0.0.0.0
PORT=8000
DEBUG=False  # Set to False in production!

# Security
API_KEY=sk-your-secret-key-here-change-this  # CRITICAL: Change this!

# CORS
CORS_ORIGINS=["http://localhost:3000"]

# Logging
LOG_LEVEL=INFO
LOG_FILE=/var/log/volumetric/app.log
```

---

## Migration Guide (For Existing Scripts)

### Old API (No validation):
```bash
# This used to work but was dangerous
curl -X POST "http://localhost:8000/api/v1/laser/power?power_w=9999"
```

### New API (With validation & auth):
```bash
# Correct way
curl -X POST "http://localhost:8000/api/v1/laser/power" \
  -H "Authorization: Bearer sk-vkte-dev-change-in-production" \
  -H "Content-Type: application/json" \
  -d '{"power_w": 5.0}'
  
# Response:
# {"status": "laser_power_set", "power_w": 5.0}
```

### Update test_api.py

```python
import os
import httpx

BASE_URL = "http://localhost:8000"
API_KEY = os.getenv("API_KEY", "sk-vkte-dev-change-in-production")

async def test_laser_power():
    async with httpx.AsyncClient(timeout=5.0) as client:
        response = await client.post(
            f"{BASE_URL}/api/v1/laser/power",
            headers={"Authorization": f"Bearer {API_KEY}"},
            json={"power_w": 5.0}
        )
        assert response.status_code == 200
```

---

## Security Checklist for Production

Before deploying to production:

- [ ] **Change API_KEY** in `.env` or environment variable
- [ ] **Set DEBUG=False** in config
- [ ] **Rotate API keys** regularly (Phase 11)
- [ ] **Use HTTPS** (add SSL certificates)
- [ ] **Restrict CORS_ORIGINS** to known domains
- [ ] **Enable authentication for WebSocket** (/ws/telemetry)
- [ ] **Set up log aggregation** (ELK, Datadog, Splunk)
- [ ] **Monitor rate limit metrics** (Phase 9)
- [ ] **Implement API versioning** for backward compatibility (Phase 7)

---

## Phase 6-7 Roadmap

- [ ] OAuth2 / OpenID Connect support
- [ ] API key scoping (read-only, write-only, device-specific)
- [ ] Sensor data encryption in transit (TLS 1.3)
- [ ] Hardware token support (USB security keys for admin)
- [ ] Audit logging (who did what, when)
- [ ] Role-based access control (RBAC)

---

## Troubleshooting

### "401 Unauthorized" response
```
Solution: Add Authorization header with valid API_KEY
curl -H "Authorization: Bearer YOUR_API_KEY" ...
```

### "422 Validation Error"
```
Solution: Check parameter types and ranges
- speed_kmh must be float 0-250
- depth_m must be float 0-500
- All values must be finite (not NaN or Inf)
```

### "429 Too Many Requests"
```
Solution: Wait 1 minute before retrying
Or use exponential backoff: 1s, 2s, 4s, 8s...
```

### Logs not appearing as JSON
```
Solution: Ensure structlog is imported and configured
Check that LOG_LEVEL=INFO in config
```

---

**Version:** 1.0  
**Status:** ✅ Implemented  
**Testing:** Ready for Phase 6 hardware integration
