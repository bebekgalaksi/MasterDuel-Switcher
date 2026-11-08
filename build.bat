@echo off
echo === Building Master Duel Switcher (Windows) ===

if not exist build mkdir build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

echo.
echo Build complete. Binary located in build\Release\MasterDuelSwitcher.exe
pause
