@echo off
SETLOCAL
set TARGET=x64

set SDL2=%CD%\libs\SDL2-2.0.5
set SDL_INC=%SDL2%\include
set SDL_LIB=%SDL2%\lib\%TARGET%

set GLM_INC=%CD%\libs\glm-0.9.8.4\glm

set CommonCompilerFlags=/Zi /Za /Od /EHsc /nologo /FC /I%SDL_INC% /I%GLM_INC%

set CommonLinkerFlags=/DEBUG /LIBPATH:%SDL_LIB% SDL2.lib SDL2main.lib

if not exist bin (
    mkdir bin
)
pushd bin

if not exist SDL2.dll (
    robocopy %SDL_LIB% . SDL2.dll
)
if not exist glew32.dll (
    robocopy %GLEW_BIN% . glew32.dll
)

cl %CommonCompilerFlags% ..\src\main.cpp /link /subsystem:console %CommonLinkerFlags% /out:snake_astar.exe
popd
echo Done