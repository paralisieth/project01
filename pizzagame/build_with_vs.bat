@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

cl.exe /EHsc /std:c++17 /MD ^
    /I"C:\vcpkg\packages\curl_x64-windows\include" ^
    /I"C:\vcpkg\packages\sqlite3_x64-windows\include" ^
    /I"C:\vcpkg\packages\nlohmann-json_x64-windows\include" ^
    /I"include" ^
    /I".." ^
    src\main.cpp src\game.cpp src\pizza.cpp src\order.cpp src\player.cpp ^
    /link ^
    /LIBPATH:"C:\vcpkg\packages\curl_x64-windows\lib" ^
    /LIBPATH:"C:\vcpkg\packages\sqlite3_x64-windows\lib" ^
    libcurl.lib sqlite3.lib shell32.lib ^
    /out:pizzarealgame.exe

if %ERRORLEVEL% EQU 0 (
    echo Build successful! Created pizzarealgame.exe
) else (
    echo Build failed with error code %ERRORLEVEL%
)