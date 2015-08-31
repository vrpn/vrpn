
msbuild quat\quatlib.sln /verbosity:detailed /p:Configuration=Release
if %errorlevel% neq 0 exit /b %errorlevel%

msbuild quat\quatlib.sln /verbosity:detailed /p:Configuration=Debug
if %errorlevel% neq 0 exit /b %errorlevel%

msbuild vrpn-vcexpress.sln /verbosity:detailed /p:Configuration=Release @vrpnsln.rsp
if %errorlevel% neq 0 exit /b %errorlevel%

msbuild vrpn-vcexpress.sln /verbosity:detailed /p:Configuration=Debug @vrpnsln.rsp
if %errorlevel% neq 0 exit /b %errorlevel%
