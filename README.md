# Chrome Cookie Retriever

This program retrieves cookies from Google Chrome's cookie database.

## Prerequisites

- CMake 3.15 or higher
- C++17 compatible compiler
- vcpkg package manager
- SQLite3

## Building the Project

1. Install dependencies using vcpkg:
```bash
vcpkg install sqlite3:x64-windows
```

2. Configure the project:
```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path_to_vcpkg]/scripts/buildsystems/vcpkg.cmake
```

3. Build the project:
```bash
cmake --build build --config Release
```

## Usage

Run the executable from the command line:
```bash
./build/Release/cookie_retriever
```

The program will display all cookies from Chrome's cookie database, including their domains, names, and values.

## Note

The program requires access to Chrome's cookie database, which is typically located at:
`%LOCALAPPDATA%\Google\Chrome\User Data\Default\Network\Cookies`

Make sure Chrome is closed when running this program to avoid database lock issues.
