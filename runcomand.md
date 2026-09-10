cd D:\4year\context-aware-schedular
cmake -S . -B build
cmake --build build --config Release
$env:Path = "C:\tools\llvm-mingw\llvm-mingw-20260616-ucrt-x86_64\bin;$env:Path"
.\build\context-aware-scheduler.exe