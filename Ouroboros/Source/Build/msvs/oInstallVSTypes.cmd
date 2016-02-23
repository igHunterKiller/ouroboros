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

if exist "%VS120COMNTOOLS%..\IDE" (
	echo Found VS120COMNTOOLS, installing usertype.dat...
	copy /Y .\usertype.dat "%VS120COMNTOOLS%..\IDE\usertype.dat"
)

if exist "%VS140COMNTOOLS%..\IDE" (
	echo Found VS140COMNTOOLS, installing usertype.dat...
	copy /Y .\usertype.dat "%VS140COMNTOOLS%..\IDE\usertype.dat"
)

pause
