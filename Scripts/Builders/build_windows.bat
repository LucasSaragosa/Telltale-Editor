@echo off

cd ../..

cmake -S . -B build -DCMAKE_PREFIX_PATH="%~1"
cmake --build build --config Release

pause