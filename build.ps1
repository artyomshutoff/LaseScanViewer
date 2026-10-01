param([string]$Toolchain = "$PSScriptRoot/.tools/llvm-mingw-20260922-ucrt-x86_64", [string]$OutputDir = 'dist')
$ErrorActionPreference='Stop'
Push-Location $PSScriptRoot
try {
    New-Item -ItemType Directory -Force $OutputDir,'build' | Out-Null
    & "$Toolchain/bin/llvm-windres.exe" app.rc -O coff -o build/app.res
    if ($LASTEXITCODE) { throw 'Resource compilation failed' }
    & "$Toolchain/bin/clang.exe" -O2 -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_DQS=0 -c src/vendor/sqlite/sqlite3.c -o build/sqlite-x64.o
    if ($LASTEXITCODE) { throw 'SQLite x64 build failed' }
    & "$Toolchain/bin/i686-w64-mingw32-clang.exe" -O2 -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_DQS=0 -c src/vendor/sqlite/sqlite3.c -o build/sqlite-x86.o
    if ($LASTEXITCODE) { throw 'SQLite x86 build failed' }
    & "$Toolchain/bin/clang++.exe" -std=c++17 -O2 -Wall -Wextra -static -municode -mwindows src/main.cpp build/app.res build/sqlite-x64.o -o "$OutputDir/LaseScanViewer.exe" -lcomctl32 -lopengl32 -lgdi32 -lcomdlg32 -lshell32 -luser32 -lole32 -lwindowscodecs -luuid
    if ($LASTEXITCODE) { throw 'Viewer build failed' }
    # A 32-bit standalone binary also runs on 64-bit Windows via WoW64.
    & "$Toolchain/bin/i686-w64-mingw32-windres.exe" app.rc -O coff -o build/app-x86.res
    if ($LASTEXITCODE) { throw 'x86 resource compilation failed' }
    & "$Toolchain/bin/i686-w64-mingw32-clang++.exe" -std=c++17 -O2 -Wall -Wextra -static -municode -mwindows src/main.cpp build/app-x86.res build/sqlite-x86.o -o "$OutputDir/LaseScanViewer-Portable.exe" -lcomctl32 -lopengl32 -lgdi32 -lcomdlg32 -lshell32 -luser32 -lole32 -lwindowscodecs -luuid
    if ($LASTEXITCODE) { throw 'Portable x86 build failed' }
    & "$Toolchain/bin/clang++.exe" -std=c++17 -O2 -Wall -Wextra -static -municode src/inspect.cpp -o build/inspect.exe
    if ($LASTEXITCODE) { throw 'Inspector build failed' }
} finally { Pop-Location }
