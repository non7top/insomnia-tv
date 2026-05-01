#!/bin/bash
set -e

# Configuration
REPO="non7top/insomnia-tv"
PORT="/dev/ttyACM0"
BAUD="460800"
WORK_DIR="/tmp/insomnia_tv_c3"

# Check dependencies
if ! command -v gh &> /dev/null; then
    echo "Error: 'gh' (GitHub CLI) is not installed."
    exit 1
fi

if ! command -v esptool.py &> /dev/null; then
    echo "Error: 'esptool.py' is not installed. Install with 'pip install esptool'."
    exit 1
fi

# Create clean work directory
rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR"
cd "$WORK_DIR"

echo "--- Fetching latest ESP32-C3 artifacts from $REPO ---"

# Download artifacts from the latest successful run of the 'Build ESP32-C3 Firmware' workflow
latest_run=$(gh run list --workflow build-c3.yml --branch master --status success --limit 1 --json databaseId --jq '.[0].databaseId')

if [ -z "$latest_run" ]; then
    echo "No successful C3 build runs found on master branch."
    exit 1
fi

echo "Found latest successful run ID: $latest_run"
gh run download "$latest_run" --name firmware-c3
gh run download "$latest_run" --name filesystem-c3

echo "--- Flashing ESP32-C3 on $PORT ---"

# Note: Adjust offsets if necessary for C3.
# For standard PlatformIO builds:
# 0x10000: Firmware
# 0x290000: LittleFS (Typical offset for min_spiffs.csv)
# We use pio's partition info if possible, but hardcoding standard C3 offsets for now.

esptool.py --chip esp32c3 --port "$PORT" --baud "$BAUD" \
    write_flash 0x10000 firmware.bin 0x290000 littlefs.bin

echo "--- Flash Complete ---"
