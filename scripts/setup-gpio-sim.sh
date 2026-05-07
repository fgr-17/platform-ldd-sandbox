#!/bin/sh
set -eu

CONFIGFS_GPIO_SIM_DIR="/sys/kernel/config/gpio-sim"
CHIP_NAME="${1:-chip0}"
BANK_NAME="${2:-bank0}"
NUM_LINES="${3:-8}"
LABEL="${4:-sandbox-gpio}"

mountpoint -q /sys/kernel/config || mount -t configfs none /sys/kernel/config

# Load module if built as loadable module. Ignore error when built-in.
modprobe gpio-sim 2>/dev/null || true

if [ ! -d "${CONFIGFS_GPIO_SIM_DIR}" ]; then
    echo "gpio-sim configfs directory not found at ${CONFIGFS_GPIO_SIM_DIR}" >&2
    echo "Check kernel config: CONFIG_GPIO_SIM and CONFIG_CONFIGFS_FS." >&2
    exit 1
fi

# Idempotent cleanup of previous chip with same name.
if [ -d "${CONFIGFS_GPIO_SIM_DIR}/${CHIP_NAME}" ]; then
    if [ -f "${CONFIGFS_GPIO_SIM_DIR}/${CHIP_NAME}/live" ]; then
        echo 0 > "${CONFIGFS_GPIO_SIM_DIR}/${CHIP_NAME}/live" 2>/dev/null || true
    fi
    rmdir "${CONFIGFS_GPIO_SIM_DIR}/${CHIP_NAME}/${BANK_NAME}" 2>/dev/null || true
    rmdir "${CONFIGFS_GPIO_SIM_DIR}/${CHIP_NAME}" 2>/dev/null || true
fi

mkdir -p "${CONFIGFS_GPIO_SIM_DIR}/${CHIP_NAME}/${BANK_NAME}"
echo "${LABEL}" > "${CONFIGFS_GPIO_SIM_DIR}/${CHIP_NAME}/${BANK_NAME}/label"
echo "${NUM_LINES}" > "${CONFIGFS_GPIO_SIM_DIR}/${CHIP_NAME}/${BANK_NAME}/num_lines"
echo 1 > "${CONFIGFS_GPIO_SIM_DIR}/${CHIP_NAME}/live"

CHIP_DEV="$(cat "${CONFIGFS_GPIO_SIM_DIR}/${CHIP_NAME}/${BANK_NAME}/chip_name")"
DEV_NAME="$(cat "${CONFIGFS_GPIO_SIM_DIR}/${CHIP_NAME}/dev_name")"

echo "gpio-sim chip created:"
echo "  configfs chip: ${CHIP_NAME}"
echo "  gpiochip dev:  /dev/${CHIP_DEV}"
echo "  platform dev:  ${DEV_NAME}"
echo "  lines:         ${NUM_LINES}"
