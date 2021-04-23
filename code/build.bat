@echo off

IF NOT EXIST "..\build" (mkdir "..\build")

pushd "..\build" 2> NUL > NUL

SET COMPILER_FLAGS=-nologo -Od -Zi -I "\Tools\SDL2\include" -DLUDUM_DEBUG=1
SET LINKER_FLAGS=-incremental:no -LIBPATH:"\Tools\SDL2\lib" SDL2d.lib user32.lib kernel32.lib


REM OpenGL shared library
REM
cl %COMPILER_FLAGS% -LD "..\code\Win_Ludum_OpenGL.cpp" -Fe"Ludum_OpenGL.dll" -link %LINKER_FLAGS% opengl32.lib

REM Main game build
REM
cl %COMPILER_FLAGS% "..\code\Win_Ludum.cpp" -Fe"Ludum.exe" -link %LINKER_FLAGS%

popd 2> NUL > NUL
