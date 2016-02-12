@echo off

rem if exist "%VS90COMNTOOLS%..\IDE" copy /Y .\autoexp.dat "%VS90COMNTOOLS%..\packages\debugger\autoexp.dat"

if exist "%VS90COMNTOOLS%..\IDE" (
	echo Found VS90COMNTOOLS, installing usertype.dat...
	copy /Y .\usertype.dat "%VS90COMNTOOLS%..\IDE\usertype.dat"
)

if exist "%VS100COMNTOOLS%..\IDE" (
	echo Found VS100COMNTOOLS, installing usertype.dat...
	copy /Y .\usertype.dat "%VS100COMNTOOLS%..\IDE\usertype.dat"
)

if exist "%VS110COMNTOOLS%..\IDE" (
	echo Found VS110COMNTOOLS, installing usertype.dat...
	copy /Y .\usertype.dat "%VS110COMNTOOLS%..\IDE\usertype.dat"
)

echo Say Yes to merge oNoStepInto.reg into your registry. This will step over several small trivial C++ objects even when stepping into using Visual Studio.

%windir%\regedit.exe .\oNoStepInto.reg
pause
