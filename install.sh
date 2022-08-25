#!/bin/bash
set -e; # Script will shutdown if an error occurs for safety reasons.
cd "$(dirname "$0")"; # Ensures location is always the folder of the install script.

git submodule init && git submodule update;

cmake -B build;
cmake --build build;
