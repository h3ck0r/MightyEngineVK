@echo off
set "SLANGC_PATH=C:/VulkanSDK/1.4.328.1/bin/slangc.exe"
set "SHADER_DIR=%~1"
set "SHADER_OUT_DIR=%~2"

echo Compiling all .slang shaders in %SHADER_DIR%...

for %%f in ("%SHADER_DIR%\*.slang") do (
    set "INPUT_FILE=%%f"
    set "OUTPUT_NAME=%%~nf.spv"
    
    echo Compiling: %%f
    
    call :COMPILE_SHADER "%%f" "%%~nf.spv"
)

echo.
echo Shader compilation complete.
goto :EOF

:COMPILE_SHADER
"%SLANGC_PATH%" %1 -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name -entry main -o "%SHADER_OUT_DIR%/%2"
    
if errorlevel 1 (
    echo ERROR: Compilation failed for %1
) else (
    echo Successfully compiled to %SHADER_DIR%/%2
)
goto :EOF