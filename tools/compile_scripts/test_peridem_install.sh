#!/usr/bin/env bash
set -euo pipefail

# Paths
SRC=/Users/prashant/Dropbox/Work/Simulations/peridem_works/PeriDEM_mac
BUILD="$SRC/build"
PREFIX=/tmp/peridem-install
CONSUMER=/tmp/peridem-consumer
CONSUMER_BUILD="$CONSUMER/build"

echo "=== Paths ==="
echo "SRC=$SRC"
echo "BUILD=$BUILD"
echo "PREFIX=$PREFIX"
echo "CONSUMER=$CONSUMER"
echo "CONSUMER_BUILD=$CONSUMER_BUILD"

# 1) remove PeriDEM build
echo "==> rm -rf \"$BUILD\""
rm -rf "$BUILD"

# 2) configure & build PeriDEM
echo "==> cmake -S \"$SRC\" -B \"$BUILD\""
cmake -S "$SRC" -B "$BUILD"
echo "==> cmake --build \"$BUILD\""
cmake --build "$BUILD" -- -j$(sysctl -n hw.ncpu)

# 3) install PeriDEM
echo "==> cmake --install \"$BUILD\" --prefix \"$PREFIX\""
cmake --install "$BUILD" --prefix "$PREFIX"

# 4) remove consumer build
echo "==> rm -rf \"$CONSUMER_BUILD\""
rm -rf "$CONSUMER_BUILD"

# 5) build consumer
echo "==> cmake -S \"$CONSUMER\" -B \"$CONSUMER_BUILD\" -DCMAKE_PREFIX_PATH=\"$PREFIX\""
cmake -S "$CONSUMER" -B "$CONSUMER_BUILD" -DCMAKE_PREFIX_PATH="$PREFIX"
echo "==> cmake --build \"$CONSUMER_BUILD\""
cmake --build "$CONSUMER_BUILD" -- -j$(sysctl -n hw.ncpu)

# 6) run consumer executable
echo "==> \"$CONSUMER_BUILD/hello\""
"$CONSUMER_BUILD/hello"
