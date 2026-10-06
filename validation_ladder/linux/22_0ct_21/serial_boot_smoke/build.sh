#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
printf '%s  %s\n' \
  '2bcc0ec62dbf3dac5d63c631effd10040442ffd3980e0b46e0de82ea978b78cf' \
  "${SCRIPT_DIR}/../c-model/boot_loader_plus_kernel.mmap" | sha256sum --check
cp "${SCRIPT_DIR}/../c-model/boot_loader_plus_kernel.mmap" "${SCRIPT_DIR}/main.mmap"
