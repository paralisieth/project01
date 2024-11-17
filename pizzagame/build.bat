@echo off
cl.exe /EHsc /std:c++17 /I"C:/vcpkg/installed/x64-windows/include" /I"include" ^
    src/main.cpp src/game.cpp src/pizza.cpp src/order.cpp src/player.cpp ^
    /link /LIBPATH:"C:/vcpkg/installed/x64-windows/lib" ^
    libcurl.lib sqlite3.lib ^
    /out:pizzarealgame.exe
