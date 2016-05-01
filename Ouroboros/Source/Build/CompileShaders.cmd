:: Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
:: This command script takes in a path to a file with several HLSL shaders, a
:: file with a list of entry points and an output directory. This will use fxc 
:: to compile each entry point to its own .h file in the output directory.

@echo off
setlocal enabledelayedexpansion

:: Set and verify arguments
set input_path=%~1
set entry_list=%~2
set output_dir=%~3
set all_args_present=true
if "!input_path!"=="" set all_args_present=false
if "!entry_list!"=="" set all_args_present=false
if "!output_dir!"=="" set all_args_present=false
if not "!all_args_present!"=="true" (
	echo usage: %~0 input_path entry_list output_dir
	echo  input_path=!input_path!
	echo  entry_list=!entry_list!
	echo  output_dir=!output_dir!
	goto end
)

:: Find FXC: before Windows 8 check for the DirectX SDK
set fxc_exe="%ProgramFiles(x86)%\Windows Kits\8.0\bin\x64\fxc.exe"
if not exist !fxc_exe! (
	if "!DXSDK_DIR!"=="" (
		echo cannot find Windows Kits or DirectX SDK
		goto end
	)

	set fxc_exe="%DXSDK_DIR%\Utilities\bin\x64\fxc.exe"
	if not exist !fxc_exe! (
		echo cannot find fxc in Windows Kits or %%DXSDK_DIR%%
		goto end
	)
)

:: Parse extra arguments to pass to fxc (first move past 3 required args)
set extra_fxc_args=
shift & shift & shift
:args_loop
if not "%1"=="" (
	set __new=!extra_fxc_args! %1
	set extra_fxc_args=!__new!
	shift
	goto args_loop
)

:: For each entry point: compile it
for /f %%i in (!entry_list!) do call:CompileShader %%i "!extra_fxc_args!"

:: For each entry point: create a master include header
call:GetBaseName "!entry_list!" entry_list_basename
set master_header="!output_dir!!entry_list_basename!.h"
if exist "!master_header!" del /f /q !master_header!
for /f %%i in (!entry_list!) do echo #include ^<%%i.h^> >> !master_header!

:: For each entry point: create a string array version
:: set tab=  
:: echo. >> !master_header!
:: echo const char* s_!entry_list_basename!_names[] = >> !master_header!
:: echo { >> !master_header!
:: for /f %%i in (!entry_list!) do echo %tab%"%%i", >> !master_header!
:: echo }; >> !master_header!

goto end

:GetBaseName
:: %1 input path, %2 is output basename
set %2=%~n1
goto end

:CompileShader
pushd .
setlocal
set entry_point=%~1
set extra_args=%~2
set output_path=!output_dir!\!entry_point!.h

:: determine the shader model from the first letter of the entry point
set entry_prefix=!entry_point:~0,1!
echo !entry_prefix! | findstr /i "V v H h D d G g P p C c" > nul
if not %ERRORLEVEL%==0 (
	echo entry point must begin with a letter indicating the type of shader it is ^(VHDGPC^)
	goto end
)

:: ensure it's all lowercase
if "!entry_prefix!"=="V" set entry_prefix=v
if "!entry_prefix!"=="H" set entry_prefix=h
if "!entry_prefix!"=="D" set entry_prefix=d
if "!entry_prefix!"=="G" set entry_prefix=g
if "!entry_prefix!"=="P" set entry_prefix=p
if "!entry_prefix!"=="C" set entry_prefix=c
set shader_model=!entry_prefix!s_5_0

if exist "!output_path!" del /f /q !output_path!
!fxc_exe! /D oHLSL !extra_args! /nologo /O3 /T !shader_model! /E !entry_point! /Vn !entry_point! /Fh "!output_path!" "!input_path!"

endlocal
popd
goto end

:end
