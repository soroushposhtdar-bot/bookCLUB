#!/usr/bin/env python3
"""BookCLUB smoke test — verifies the client launches and connects to the server.

Usage:
    python3 scripts/smoke_test.py

Prerequisites:
    - Build the project first (mkdir build && cd build && cmake .. && cmake --build . -j8)
    - No need to start the server manually — this script starts it for you.

What it does:
    1. Starts the server in the background.
    2. Waits 2 seconds for the server to initialize.
    3. Starts the client with QT_QPA_PLATFORM=offscreen.
    4. Waits up to 10 seconds for the client to load QML successfully.
    5. Verifies the client produced the "BookClub client ready" log message.
    6. Kills both processes and exits 0 on success, 1 on failure.

For manual end-to-end verification of the auth flow:
    1. Start the server: scripts/run_server.sh
    2. Start the client: scripts/run_client.sh
    3. Log in with one of the demo accounts:
         admin / admin (Admin role)
         publisher1 / publisher1 (Publisher role)
         amir / amir1234 (User role)
"""
import os, subprocess, sys, time, pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
BUILD_DIR = ROOT / "build"
BIN_DIR = BUILD_DIR / "bin"
SERVER = BIN_DIR / "BookClubServer"
CLIENT = BIN_DIR / "BookClubClient"

def main():
    if not SERVER.exists() or not CLIENT.exists():
        print(f"ERROR: Binaries not found.")
        print(f"  Expected: {SERVER}")
        print(f"  Expected: {CLIENT}")
        print(f"Build the project first:")
        print(f"  cd {ROOT}")
        print(f"  mkdir -p build && cd build")
        print(f"  cmake ..")
        print(f"  cmake --build . -j8")
        return 1

    env = os.environ.copy()
    env["QT_QPA_PLATFORM"] = "offscreen"
    # Suppress verbose QML import logging
    env["QT_LOGGING_RULES"] = "qt.qml.import=false"

    # Start the server
    print("[smoke] starting server...")
    server = subprocess.Popen(
        [str(SERVER), "-p", "8080"],
        cwd=str(ROOT),
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        env=env,
    )
    try:
        time.sleep(2.0)
        if server.poll() is not None:
            out = server.stdout.read().decode(errors="replace")
            print(f"[smoke] FAIL: server exited early")
            print(out)
            return 1

        # Verify the server is listening on port 8080
        print("[smoke] verifying server is listening on port 8080...")
        try:
            import socket
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(1.0)
            s.connect(("127.0.0.1", 8080))
            s.close()
            print("[smoke] server is listening on port 8080")
        except Exception as e:
            print(f"[smoke] FAIL: server is not listening on port 8080: {e}")
            return 1

        # Start the client
        print("[smoke] starting client...")
        client = subprocess.Popen(
            [str(CLIENT)],
            cwd=str(ROOT),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            env=env,
        )
        try:
            # Wait up to 10s for the "BookClub client ready" log line
            deadline = time.time() + 10.0
            output_lines = []
            success = False
            while time.time() < deadline:
                if client.poll() is not None:
                    rest = client.stdout.read().decode(errors="replace")
                    output_lines.append(rest)
                    break
                line = client.stdout.readline().decode(errors="replace")
                if not line:
                    time.sleep(0.1)
                    continue
                output_lines.append(line)
                sys.stdout.write(f"[client] {line}")
                sys.stdout.flush()
                if "BookClub client ready" in line:
                    success = True
                    break
                if "Failed to load QML root" in line:
                    print(f"[smoke] FAIL: client failed to load QML root")
                    print("".join(output_lines))
                    return 1

            if not success:
                print(f"[smoke] FAIL: client did not produce 'BookClub client ready' within 10s")
                print("".join(output_lines))
                return 1

            print(f"[smoke] PASS: client loaded QML successfully")
            print(f"[smoke] The full auth flow can be verified manually:")
            print(f"[smoke]   1. Run scripts/run_server.sh")
            print(f"[smoke]   2. Run scripts/run_client.sh")
            print(f"[smoke]   3. Log in with: admin/admin, publisher1/publisher1, or amir/amir1234")
            return 0
        finally:
            if client.poll() is None:
                client.terminate()
                try:
                    client.wait(timeout=2.0)
                except subprocess.TimeoutExpired:
                    client.kill()
    finally:
        if server.poll() is None:
            server.terminate()
            try:
                server.wait(timeout=2.0)
            except subprocess.TimeoutExpired:
                server.kill()


if __name__ == "__main__":
    sys.exit(main())
