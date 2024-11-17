@echo off
echo Copying required DLLs...

copy "C:\vcpkg\packages\curl_x64-windows\bin\libcurl.dll" .
copy "C:\vcpkg\packages\sqlite3_x64-windows\bin\sqlite3.dll" .
copy "C:\vcpkg\packages\zlib_x64-windows\bin\zlib1.dll" .

:: Try both potential OpenSSL paths
if exist "C:\vcpkg\packages\openssl_x64-windows\bin\libcrypto-3-x64.dll" (
    copy "C:\vcpkg\packages\openssl_x64-windows\bin\libcrypto-3-x64.dll" .
    copy "C:\vcpkg\packages\openssl_x64-windows\bin\libssl-3-x64.dll" .
) else if exist "C:\vcpkg\packages\openssl_x64-windows\bin\libcrypto-1_1-x64.dll" (
    copy "C:\vcpkg\packages\openssl_x64-windows\bin\libcrypto-1_1-x64.dll" .
    copy "C:\vcpkg\packages\openssl_x64-windows\bin\libssl-1_1-x64.dll" .
)

echo Done copying DLLs!