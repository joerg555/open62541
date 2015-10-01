rem @echo OFF
rem if Python is in an other path please set PYTHON_EXECUTABLE environment
if exist c:\Python27\python.exe set PYTHON_EXECUTABLE=c:\Python27\python.exe
if not (%PYTHON_EXECUTABLE%) == () goto ok
  set KEY_NAME=HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\Python.exe
  for /F "usebackq tokens=3" %%A IN (`reg query "%KEY_NAME%" /ve 2^>nul `) do set PYTHON_EXECUTABLE=%%A
:ok
rem echo %PYTHON_EXECUTABLE%

cd %~dp0
set PROJECT_SOURCE_DIR=%~dp0..\
set PROJECT_BINARY_DIR=%PROJECT_SOURCE_DIR%
set ENABLE_SUBSCRIPTIONS="--enable-subscription-types=1"

if not exist src_generated mkdir src_generated

rem if not exist src_generated\ua_config.h echo generate def. ua_config.h & copy include\ua_configw32.h src_generated\ua_config.h
rem if not exist src_generated\ua_config.h echo generate def. ua_config.h & 
powershell -command Get-Content ..\include\ua_config.h.in ^| Foreach-Object { ^
$x = $_ -replace '\${UA_LOGLEVEL}', '100'; ^
$x = $x -replace '#cmakedefine UA_MULTITHREADING', '// #define UA_MULTITHREADING'; ^
$x = $x -replace '#cmakedefine ENABLE_METHODCALLS', '// #define ENABLE_METHODCALLS'; ^
$x = $x -replace '#cmakedefine ENABLE_SUBSCRIPTIONS', '#define ENABLE_SUBSCRIPTIONS'; ^
$x = $x -replace '#cmakedefine ENABLE_TYPEINTROSPECTION', '// #define ENABLE_TYPEINTROSPECTION'; ^
$x = $x -replace '__declspec\(dllimport\)', ''; ^
$x} > src_generated\ua_config.h

echo Generate src_generated/ua_nodeids
%PYTHON_EXECUTABLE% ../tools/generate_nodeids.py ..\tools/schema/NodeIds.csv src_generated/ua_nodeids
echo Generate src_generated/ua_types
%PYTHON_EXECUTABLE% ../tools/generate_datatypes.py %ENABLE_SUBSCRIPTIONS% --typedescriptions ../tools/schema/NodeIds.csv 0 ../tools/schema/Opc.Ua.Types.bsd src_generated/ua_types
echo Generate src_generated/ua_transport
%PYTHON_EXECUTABLE% ../tools/generate_datatypes.py --ns0-types-xml ../tools/schema/Opc.Ua.Types.bsd 1 ../tools/schema/Custom.Opc.Ua.Transport.bsd src_generated/ua_transport
