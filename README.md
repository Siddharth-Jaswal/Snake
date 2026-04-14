# Snake Game with SFML

Simple Snake implementation in C++ using SFML with:

- Start screen and game-over screen
- Score display
- Increasing speed as score rises
- Grid-based movement and rendering
- Smooth interpolation between movement steps

## Build

### CMake

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSFML_DIR="C:/path/to/SFML/lib/cmake/SFML"
cmake --build build
```

### Run

```powershell
.\build\SnakeSFML.exe
```

For multi-config generators, the executable may instead be under a configuration folder such as:

```powershell
.\build\Release\SnakeSFML.exe
```

## SFML Setup

The project targets SFML 3.x and expects the SFML development libraries to be installed and discoverable by CMake.

### Recommended Windows Downloads

For a matching Windows GCC + SFML setup, these downloads work well together:

- MinGW-w64 GCC 14.2.0 (WinLibs, UCRT, POSIX, SEH):
  https://github.com/brechtsanders/winlibs_mingw/releases/download/14.2.0posix-19.1.1-12.0.0-ucrt-r2/winlibs-x86_64-posix-seh-gcc-14.2.0-mingw-w64ucrt-12.0.0-r2.7z
- SFML 3.0.2 for GCC 14.2.0 MinGW 64-bit:
  https://www.sfml-dev.org/files/SFML-3.0.2-windows-gcc-14.2.0-mingw-64-bit.zip

Suggested layout after extracting:

```text
C:/tools/mingw64
C:/tools/SFML-3.0.2
```

On Windows, common approaches are:

- Point `SFML_DIR` directly at the SFML CMake package:

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSFML_DIR="C:/path/to/SFML/lib/cmake/SFML"
```

- Or point `CMAKE_PREFIX_PATH` at the SFML installation root:

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:/path/to/SFML"
```

- Or use a package manager such as `vcpkg` and configure CMake accordingly.

## Windows MinGW + SFML Runtime Notes

Incorrect compiler, runtime, or DLL setup on Windows can cause runtime failures such as:

- `Entry Point Not Found (codecvt_utf8_utf16)`
- missing `sfml-graphics-d-3.dll`
- generic DLL not found errors

These are usually environment issues, not code bugs.

### Root Causes

#### 1. Compiler and SFML mismatch

SFML binaries must match the compiler toolchain used to build the project:

- same compiler family
- same GCC version
- same runtime variant, such as UCRT

If your project uses MinGW-w64 GCC built for UCRT, SFML should also be built for that same MinGW/UCRT combination.

#### 2. Debug and Release mismatch

SFML ships separate debug and release DLLs:

- Debug: `sfml-*-d-3.dll`
- Release: `sfml-*-3.dll`

Do not mix them:

- Debug build with release DLLs: invalid
- Release build with debug DLLs: invalid

#### 3. Multiple C++ runtimes on PATH

Windows may load the wrong `libstdc++-6.dll` from another tool installation on `PATH`.

That can lead to symbol lookup or entry-point errors even when the project itself is built correctly.

## Recommended Setup

### 1. Use a matching compiler

Install a MinGW-w64 distribution that matches the SFML binaries you plan to use.

Verify your compiler:

```powershell
g++ --version
```

### 2. Use matching SFML binaries

Download or build SFML 3.x for the same compiler and runtime family as your MinGW toolchain.

### 3. Build in Release mode

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSFML_DIR="C:/path/to/SFML/lib/cmake/SFML"
cmake --build build
```

### 4. Copy required SFML DLLs next to the executable

Copy these from your SFML `bin` directory into the output directory that contains `SnakeSFML.exe`:

```text
sfml-system-3.dll
sfml-window-3.dll
sfml-graphics-3.dll
```

### 5. Copy the MinGW runtime DLLs next to the executable

Copy these from your MinGW `bin` directory into the same output directory:

```text
libstdc++-6.dll
libgcc_s_seh-1.dll
libwinpthread-1.dll
```

This prevents Windows from loading incompatible runtime DLLs from unrelated tools on `PATH`.

## Important Notes

- Do not rely on global `PATH` resolution for SFML or MinGW runtime DLLs.
- Do not mix debug and release artifacts.
- Do not mix multiple unrelated MinGW distributions unless you know the runtime implications.
- Keeping all required DLLs next to the executable is the most reliable setup on Windows.

## Debugging Commands

Check which runtime DLLs are visible:

```powershell
where libstdc++-6.dll
```

Inspect executable dependencies:

```powershell
objdump -p build/SnakeSFML.exe | findstr sfml
```

## Controls

- Arrow keys control the snake
- `W`, `A`, `S`, `D` also control the snake
- `Enter` starts or restarts
- `Esc` exits

## Notes

- The game tries to load a font from `assets/fonts/` first, then falls back to common system font paths.

## itch.io Release

For a Windows itch.io upload, package the built executable together with the required runtime DLLs in a single folder.

Recommended bundle contents:

```text
SnakeSFML.exe
sfml-system-3.dll
sfml-window-3.dll
sfml-graphics-3.dll
libstdc++-6.dll
libgcc_s_seh-1.dll
libwinpthread-1.dll
README.txt
```

Upload either:

- a `.zip` containing those files at the root, or
- the folder itself if you are using `butler`

Suggested itch.io settings:

- Platform: `Windows`
- Kind of project: `Downloadable`
- Classification: `Game`

If you want the game to launch directly from the itch app, keep the executable at the top level of the uploaded archive and set the launch target to `SnakeSFML.exe`.
