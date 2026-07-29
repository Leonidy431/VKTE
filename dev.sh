#!/bin/bash
# Quick dev server launcher for Volumetric Display backend

set -e

echo "🚀 Volumetric Display Dev Server"
echo "================================"
echo ""

# Check Docker
if ! command -v docker &> /dev/null; then
    echo "❌ Docker not found. Install Docker Desktop or Docker Engine."
    exit 1
fi

# Check docker daemon
if ! docker ps &> /dev/null; then
    echo "❌ Docker daemon not running. Start Docker and try again."
    exit 1
fi

echo "✅ Docker available"
echo ""

# Navigate to project root
cd "$(dirname "$0")"

echo "🐳 Building and starting containers..."
docker compose up --build backend -d

echo ""
echo "⏳ Waiting for backend to be healthy (30s)..."
for i in {1..30}; do
    if curl -sf http://localhost:8000/health > /dev/null 2>&1; then
        echo "✅ Backend is healthy!"
        break
    fi
    echo "  Attempt $i/30..."
    sleep 1
done

echo ""
echo "📊 System Status:"
docker compose exec backend curl -s http://localhost:8000/api/v1/status | python3 -m json.tool 2>/dev/null || echo "  (Could not fetch status)"

echo ""
echo "🌐 API Endpoints:"
echo "  Health:    http://localhost:8000/health"
echo "  Status:    http://localhost:8000/api/v1/status"
echo "  Docs:      http://localhost:8000/docs (Swagger UI)"
echo ""
echo "📡 WebSocket:"
echo "  Telemetry: ws://localhost:8000/ws/telemetry"
echo ""
echo "📋 Logs:"
echo "  docker compose logs -f backend"
echo ""
echo "🛑 Stop:"
echo "  docker compose down"
echo ""
