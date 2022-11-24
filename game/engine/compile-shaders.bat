@ECHO OFF
%VULKAN_SDK%/Bin/glslc.exe src/shaders/shader.vert -o src/shaders/vert.spv
%VULKAN_SDK%/Bin/glslc.exe src/shaders/shader.frag -o src/shaders/frag.spv

ECHO "Building shaders"