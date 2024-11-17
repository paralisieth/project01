@echo off
echo Building Chrome Cookie Retriever...

:: Initialize Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

:: Create and enter build directory
if not exist build mkdir build
cd build

:: Configure with CMake
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

:: Build the project
cmake --build . --config Release

:: Copy the executable to the root directory
copy /Y Release\cookie_retriever.exe ..\cookie_retriever.exe

:: Copy required DLLs
echo Copying required DLLs...
copy /Y "C:\vcpkg\packages\curl_x64-windows\bin\libcurl.dll" ..\
copy /Y "C:\vcpkg\packages\sqlite3_x64-windows\bin\sqlite3.dll" ..\
copy /Y "C:\vcpkg\packages\zlib_x64-windows\bin\zlib1.dll" ..\

echo Build complete! You can now run cookie_retriever.exe
