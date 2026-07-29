#!/bin/bash
# ============================================================
# BookCLUB — Start the client
# ============================================================
# Usage: ./scripts/run_client.sh
# Prerequisite: the server must already be running (./scripts/run_server.sh)
# ============================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BIN_DIR="${SCRIPT_DIR}/../build/bin"

# Fall back to project-root-relative path if build/bin doesn't exist
if [ ! -x "${BIN_DIR}/BookClubClient" ]; then
    BIN_DIR="${PROJECT_ROOT}/build/bin"
fi

if [ ! -x "${BIN_DIR}/BookClubClient" ]; then
    echo "Error: BookClubClient not found."
    echo "Build the project first:"
    echo "  cd ${PROJECT_ROOT}"
    echo "  mkdir -p build && cd build"
    echo "  cmake .."
    echo "  cmake --build . -j8"
    exit 1
fi

# Run from project root so the client and server share the working directory
cd "${PROJECT_ROOT}"
echo "Starting BookClubClient..."
exec "${BIN_DIR}/BookClubClient"
