REM Build script for engine
@ECHO OFF
SetLocal EnableDelayedExpansion

REM Get a list of all the .c files.
SET cFilenames=
FOR /R %%f in (*.cpp) do (
    SET cFilenames=!cFilenames! %%f
)

REM echo "Files:" %cFilenames%

SET assembly=engine
SET compilerFlags=-g -Wvarargs -Wall -Werror
REM -Wall -Werror
SET includeFlags=-std=c++17 -Isrc -Isrc/vendor -I%VULKAN_SDK%/Include
SET linkerFlags=-luser32 -lglfw3 -lgdi32  -lvulkan-1 -L%VULKAN_SDK%/Lib
SET defines=-D_DEBUG -DEM_EXPORT -D_CRT_SECURE_NO_WARNINGS

ECHO "Building %assembly%%..."
g++ %cFilenames% %compilerFlags% -o ../bin/%assembly%.exe %defines% %includeFlags% %linkerFlags% && start ../bin/%assembly%.exe