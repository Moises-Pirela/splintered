@ECHO OFF

SET vertShaders=
FOR /R %%f in (*.vert) do (
    echo %%~nf.spv
    SET vertShaders=!vertShaders! %%f
    %VULKAN_SDK%/Bin/glslc.exe %%f -o src/shaders/%%~nf.spv
)

SET fragShaders=
FOR /R %%f in (*.frag) do (
    echo %%~nf
     SET fragShaders=!fragShaders! %%f
    %VULKAN_SDK%/Bin/glslc.exe %%f -o src/shaders/%%~nf.spv   
)

ECHO "Building shaders"