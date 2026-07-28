#!/bin/bash
# ============================================================
# BookCLUB — Linux/macOS launch script
# ============================================================
# Starts the server, waits 2 seconds, then starts the client.
# ============================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_DIR="${SCRIPT_DIR}/../build/bin"

# Start the server in the background
echo "Starting BookClubServer on port 8080..."
"${BIN_DIR}/BookClubServer" -p 8080 &
SERVER_PID=$!

# Wait for the server to initialise
sleep 2

# Start the client
echo "Starting BookClubClient..."
"${BIN_DIR}/BookClubClient"
CLIENT_EXIT=$?

# When the client exits, kill the server
kill $SERVER_PID 2>/dev/null || true
wait $SERVER_PID 2>/dev/null || true

exit $CLIENT_EXIT
