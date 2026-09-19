#!/bin/sh
set -eu
/app/smart_server &
server_pid=$!
python3 /app/dashboard.py &
web_pid=$!
trap 'kill "$server_pid" "$web_pid" 2>/dev/null || true; wait 2>/dev/null || true' TERM INT
wait "$server_pid"
kill "$web_pid" 2>/dev/null || true
wait "$web_pid" 2>/dev/null || true
