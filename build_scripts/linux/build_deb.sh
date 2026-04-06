#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

if [[ ! -f "${ROOT_DIR}/make_deb.sh" ]]; then
  echo "make_deb.sh not found in project root"
  exit 1
fi

"${ROOT_DIR}/make_deb.sh"
