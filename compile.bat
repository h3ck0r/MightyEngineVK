@echo off
set "GLSLC_PATH=C:/VulkanSDK/1.4.328.1/bin/glslc.exe"
set "SHADER_DIR=%~1"
set "SHADER_OUT_DIR=%~2"

echo Compiling all .slang shaders in %SHADER_DIR%...

if not exist "%SHADER_OUT_DIR%" mkdir "%SHADER_OUT_DIR%"
for %%e in (.rgen, .rmiss, .rchit) do (
    for %%f in ("%SHADER_DIR%\*%%e") do (
        set "INPUT_FILE=%%f"
        set "OUTPUT_NAME=%%~nf%%e.spv"
        
        echo Compiling: %%f
        
        call :COMPILE_SHADER "%%f" %%~nf%%e.spv
    )
)

echo.
echo Shader compilation complete.
goto :EOF

:COMPILE_SHADER
"%GLSLC_PATH%" %1 -o "%SHADER_OUT_DIR%\%2"
    
if errorlevel 1 (
    echo ERROR: Compilation failed for %1
) else (
    echo Successfully compiled to %SHADER_DIR%/%2
)
goto :EOF