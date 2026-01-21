#!/bin/bash

# Configuration
FQBN="m5stack:esp32:m5stack_stickc_plus2"

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Function to find ESP32 port
find_esp32_port() {
  # Common ESP32 port patterns on macOS
  local patterns=(
    "/dev/tty.usbserial-*"
    "/dev/tty.usbmodem*"
    "/dev/cu.usbserial-*"
    "/dev/cu.usbmodem*"
  )
  
  local found_ports=()
  
  for pattern in "${patterns[@]}"; do
    for port in $pattern; do
      if [ -e "$port" ] && [ -c "$port" ]; then
        found_ports+=("$port")
      fi
    done
  done
  
  if [ ${#found_ports[@]} -eq 0 ]; then
    echo -e "${RED}No ESP32 port found!${NC}" >&2
    echo -e "${YELLOW}Please check:${NC}" >&2
    echo -e "  1. ESP32 device is connected via USB" >&2
    echo -e "  2. USB cable supports data transfer" >&2
    echo -e "  3. Device drivers are installed" >&2
    exit 1
  elif [ ${#found_ports[@]} -eq 1 ]; then
    echo "${found_ports[0]}"
  else
    echo -e "${YELLOW}Multiple ports found:${NC}" >&2
    for i in "${!found_ports[@]}"; do
      echo -e "  $((i+1)). ${found_ports[$i]}" >&2
    done
    echo -e "${YELLOW}Using first port: ${found_ports[0]}${NC}" >&2
    echo "${found_ports[0]}"
  fi
}

# Auto-detect port
echo -e "${YELLOW}Detecting ESP32 port...${NC}"
PORT=$(find_esp32_port)
if [ $? -ne 0 ]; then
  exit 1
fi
echo -e "${GREEN}Found port: $PORT${NC}"

echo -e "${YELLOW}Step 0: Source virtual environment...${NC}"
source bin/activate

if [ $? -ne 0 ]; then
  echo -e "${RED}Failed to activate virtual environment!${NC}"
  exit 1
fi

echo -e "${GREEN}Virtual environment activated!${NC}"

echo -e "${YELLOW}Step 1: Compiling...${NC}"
arduino-cli compile --fqbn $FQBN -e \
  --build-property build.partitions=no_ota \
  --build-property upload.maximum_size=3145728 \
  --build-property compiler.c.extra_flags="-Os" \
  --build-property compiler.cpp.extra_flags="-Os" \
  ./m5stick-nemo.ino

if [ $? -ne 0 ]; then
  echo -e "${RED}Compilation failed!${NC}"
  exit 1
fi

echo -e "${GREEN}Compilation successful!${NC}"

# Find the build directory
BUILD_DIR=$(find ~/Library/Caches/arduino -name "m5stick-nemo.ino.bin" -type f | head -1 | xargs dirname)

if [ -z "$BUILD_DIR" ]; then
  echo -e "${RED}Could not find build directory!${NC}"
  exit 1
fi

echo -e "${YELLOW}Step 2: Found build directory: $BUILD_DIR${NC}"
cd "$BUILD_DIR"

echo -e "${YELLOW}Step 3: Flashing bootloader...${NC}"
esptool.py --chip esp32 --port $PORT write_flash 0x1000 m5stick-nemo.ino.bootloader.bin

if [ $? -ne 0 ]; then
  echo -e "${RED}Bootloader flash failed!${NC}"
  exit 1
fi

echo -e "${YELLOW}Step 4: Flashing partitions...${NC}"
esptool.py --chip esp32 --port $PORT write_flash 0x8000 m5stick-nemo.ino.partitions.bin

if [ $? -ne 0 ]; then
  echo -e "${RED}Partitions flash failed!${NC}"
  exit 1
fi

echo -e "${YELLOW}Step 5: Flashing application...${NC}"
esptool.py --chip esp32 --port $PORT write_flash 0x10000 m5stick-nemo.ino.bin

if [ $? -ne 0 ]; then
  echo -e "${RED}Application flash failed!${NC}"
  exit 1
fi

echo -e "${GREEN}Flash complete!${NC}"
