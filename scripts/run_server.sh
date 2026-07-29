#!/bin/bash
# ============================================================
# BookCLUB — Start the server
# ============================================================
# Usage: ./scripts/run_server.sh [port]
# Default port: 8080
# ============================================================

set -e

PORT="${1:-8080}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BIN_DIR="${SCRIPT_DIR}/../build/bin"

# Fall back to project-root-relative path if build/bin doesn't exist
if [ ! -x "${BIN_DIR}/BookClubServer" ]; then
    BIN_DIR="${PROJECT_ROOT}/build/bin"
fi

if [ ! -x "${BIN_DIR}/BookClubServer" ]; then
    echo "Error: BookClubServer not found."
    echo "Build the project first:"
    echo "  cd ${PROJECT_ROOT}"
    echo "  mkdir -p build && cd build"
    echo "  cmake .."
    echo "  cmake --build . -j8"
    exit 1
fi

# Run the server from the project root so it can find database/schema.sql
cd "${PROJECT_ROOT}"
echo "Starting BookClubServer on port ${PORT}..."
exec "${BIN_DIR}/BookClubServer" -p "${PORT}"
