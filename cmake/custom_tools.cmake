
set(CS_TOOL_PATH ${CMAKE_SOURCE_DIR}/tools)

message(STATUS "Loading tools in ${CS_TOOL_PATH}")
#Find program CSBridge.exe
find_program(TOOL_CSBRIDGE NAMES CSBridge.exe PATHS ${CS_TOOL_PATH}/bin/)

# Check if dotnet is in PATH
find_program(TOOL_DOTNET NAMES dotnet)

message(STATUS "CSBridge executable found at: ${TOOL_CSBRIDGE}")
message(STATUS "dotnet sdk found at: ${TOOL_DOTNET}")

