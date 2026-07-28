#!/bin/bash
# Demo script: Complete workflow of the STM32H745 Shotgun Test Bench CLI
# This script demonstrates all major features of the test bench software.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  STM32H745 Shotgun Test Bench - Complete CLI Demo             ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

echo "📊 PHASE 1: Simulate Multiple Ammunition Groups"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo "Generating factory ammunition group (2.5gr, 15 shots)..."
python3 cli.py mock --shots 15 --session "factory_group_1" --ammo "factory_2.5gr" \
    --distance 25.0 --output demo_factory_1.json > /dev/null
echo "✓ Factory group 1 saved"
echo ""

echo "Generating reloaded ammunition group (2.6gr, 15 shots)..."
python3 cli.py mock --shots 15 --session "reload_group_1" --ammo "reload_2.6gr" \
    --distance 25.0 --output demo_reload_1.json > /dev/null
echo "✓ Reload group 1 saved"
echo ""

echo ""
echo "📈 PHASE 2: Analyze Factory Ammunition"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
python3 cli.py analyze demo_factory_1.json --thermal --recoil
echo ""

echo ""
echo "📈 PHASE 3: Analyze Reloaded Ammunition"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
python3 cli.py analyze demo_reload_1.json --thermal --recoil
echo ""

echo ""
echo "🔍 PHASE 4: Data Validation"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "Validating factory group..."
python3 cli.py validate demo_factory_1.json
echo ""

echo "Validating reload group..."
python3 cli.py validate demo_reload_1.json
echo ""

echo ""
echo "💾 PHASE 5: Export to CSV"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
python3 cli.py export demo_factory_1.json --format csv --output demo_factory_1.csv
python3 cli.py export demo_reload_1.json --format csv --output demo_reload_1.csv
echo ""

echo ""
echo "📋 PHASE 6: Data Comparison"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Extract key stats from both groups
factory_stats=$(python3 -c "
import json
with open('demo_factory_1.json') as f:
    s = json.load(f)
    shots = s['shots']
    temps = [float(shot['barrel_temp_c']) for shot in shots]
    recoils = [float(shot['recoil_peak_g']) for shot in shots]
    print(f\"Temp range: {min(temps):.1f}-{max(temps):.1f}°C\")
    print(f\"Avg recoil: {sum(recoils)/len(recoils):.2f}g\")
")

reload_stats=$(python3 -c "
import json
with open('demo_reload_1.json') as f:
    s = json.load(f)
    shots = s['shots']
    temps = [float(shot['barrel_temp_c']) for shot in shots]
    recoils = [float(shot['recoil_peak_g']) for shot in shots]
    print(f\"Temp range: {min(temps):.1f}-{max(temps):.1f}°C\")
    print(f\"Avg recoil: {sum(recoils)/len(recoils):.2f}g\")
")

echo "Factory (2.5gr):"
echo "$factory_stats" | sed 's/^/  /'
echo ""
echo "Reloaded (2.6gr):"
echo "$reload_stats" | sed 's/^/  /'
echo ""

echo ""
echo "✅ Demo Complete!"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "Generated files:"
ls -lh demo_*.json demo_*.csv 2>/dev/null | awk '{print "  " $9 " (" $5 ")"}'
echo ""
echo "Next steps:"
echo "  • Review CSV exports in a spreadsheet application"
echo "  • Use 'cli.py acquire <port>' for live hardware testing"
echo "  • Check 'cli.py --help' for all available commands"
echo ""
