@echo OFF
rem if Python is in an other path please set PYTHON_EXECUTABLE environment
if exist c:\Python27\python.exe set PYTHON_EXECUTABLE=c:\Python27\python.exe
if not (%PYTHON_EXECUTABLE%) == () goto ok
  set KEY_NAME=HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\Python.exe
  for /F "usebackq tokens=3" %%A IN (`reg query "%KEY_NAME%" /ve 2^>nul `) do set PYTHON_EXECUTABLE=%%A
:ok
echo %PYTHON_EXECUTABLE%

cd %~dp0
set PROJECT_SOURCE_DIR=%~dp0..
set GIT_COMMIT_ID="1.0"

echo Die Vereinigung
set exported_headers=./src_generated/ua_config.h 
 set exported_headers=%exported_headers% ../include/ua_statuscodes.h 
 set exported_headers=%exported_headers% ../include/ua_types.h 
 set exported_headers=%exported_headers% ./src_generated/ua_nodeids.h 
 set exported_headers=%exported_headers% ./src_generated/ua_types_generated.h 
 set exported_headers=%exported_headers% ../include/ua_connection.h 
 set exported_headers=%exported_headers% ../include/ua_log.h 
 set exported_headers=%exported_headers% ../include/ua_server.h 
 set exported_headers=%exported_headers% ../include/ua_client.h 
 set exported_headers=%exported_headers% ../examples/networklayer_tcp.h 
 set exported_headers=%exported_headers% ../examples/logger_stdout.h

set internal_headers=../src/ua_util.h
 set internal_headers=%internal_headers% ../deps/queue.h
 set internal_headers=%internal_headers% ../deps/pcg_basic.h
 set internal_headers=%internal_headers% ../src/ua_types_encoding_binary.h
 set internal_headers=%internal_headers% ./src_generated/ua_types_generated_encoding_binary.h
 set internal_headers=%internal_headers% ./src_generated/ua_transport_generated.h
 set internal_headers=%internal_headers% ./src_generated/ua_transport_generated_encoding_binary.h
 set internal_headers=%internal_headers% ../src/ua_securechannel.h
 set internal_headers=%internal_headers% ../src/server/ua_nodes.h
 set internal_headers=%internal_headers% ../src/ua_session.h
 set internal_headers=%internal_headers% ../src/server/ua_nodestore.h
 set internal_headers=%internal_headers% ../src/server/ua_session_manager.h
 set internal_headers=%internal_headers% ../src/server/ua_securechannel_manager.h
 set internal_headers=%internal_headers% ../src/server/ua_server_internal.h
 set internal_headers=%internal_headers% ../src/server/ua_services.h
 set internal_headers=%internal_headers% ../src/client/ua_client_internal.h

set lib_sources=../src/ua_types.c
 set lib_sources=%lib_sources% ../src/ua_types_encoding_binary.c
 set lib_sources=%lib_sources% ./src_generated/ua_types_generated.c
 set lib_sources=%lib_sources% ./src_generated/ua_transport_generated.c
 set lib_sources=%lib_sources% ../src/ua_connection.c
 set lib_sources=%lib_sources% ../src/ua_securechannel.c
 set lib_sources=%lib_sources% ../src/ua_session.c
 set lib_sources=%lib_sources% ../src/server/ua_server.c
 set lib_sources=%lib_sources% ../src/server/ua_server_addressspace.c
 set lib_sources=%lib_sources% ../src/server/ua_server_binary.c
 set lib_sources=%lib_sources% ../src/server/ua_nodes.c
 set lib_sources=%lib_sources% ../src/server/ua_server_worker.c
 set lib_sources=%lib_sources% ../src/server/ua_securechannel_manager.c
 set lib_sources=%lib_sources% ../src/server/ua_session_manager.c
 set lib_sources=%lib_sources% ../src/server/ua_services_discovery.c
 set lib_sources=%lib_sources% ../src/server/ua_services_securechannel.c
 set lib_sources=%lib_sources% ../src/server/ua_services_session.c
 set lib_sources=%lib_sources% ../src/server/ua_services_attribute.c
 set lib_sources=%lib_sources% ../src/server/ua_services_nodemanagement.c
 set lib_sources=%lib_sources% ../src/server/ua_services_view.c
 set lib_sources=%lib_sources% ../src/client/ua_client.c
 set lib_sources=%lib_sources% ../examples/networklayer_tcp.c
 set lib_sources=%lib_sources% ../examples/logger_stdout.c
 set lib_sources=%lib_sources% ../deps/pcg_basic.c

%PYTHON_EXECUTABLE% ../tools/amalgamate.py %GIT_COMMIT_ID% open62541.h %exported_headers%
%PYTHON_EXECUTABLE% ../tools/amalgamate.py %GIT_COMMIT_ID% open62541.c %internal_headers% ../src/server/ua_nodestore_hash.inc %lib_sources%

