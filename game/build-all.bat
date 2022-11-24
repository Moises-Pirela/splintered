@ECHO OFF
REM Build Everything

ECHO "Building everything..."


PUSHD engine
CALL build.bat
IF %ERRORLEVEL% NEQ 0 (echo Error:%ERRORLEVEL% && exit )
CALL compile-shaders.bat
POPD

ECHO "All assemblies built successfully."