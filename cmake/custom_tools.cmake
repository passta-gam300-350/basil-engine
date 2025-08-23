

set(CS_TOOL_PATH ${CMAKE_SOURCE_DIR}/tools)
#Find program CSBridge.exe
find_program(CSBridger NAMES CSBridge.exe PATHS ${CS_TOOL_PATH}/bin/
)


message(STATUS "CSBridge executable found at: ${CSBridger}")
