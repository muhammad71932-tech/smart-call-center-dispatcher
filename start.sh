#!/bin/bash
cd "$(dirname "$0")/backend"

# Kill any previous instance on port 9090
OLD_PID=$(lsof -ti:9090 2>/dev/null)
if [ -n "$OLD_PID" ]; then
  echo "  Stopping previous instance (PID $OLD_PID)..."
  kill "$OLD_PID" 2>/dev/null
  sleep 0.5
fi

make -s
if [ $? -ne 0 ]; then
  echo "Build failed. Check output above."
  exit 1
fi
echo ""
echo "  Smart Call Center Dispatcher"
echo "  =============================="
echo "  Open your browser at: http://localhost:9090"
echo ""
./dispatcher
