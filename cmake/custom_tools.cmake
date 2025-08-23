
#Find program CSBridge.exe
find_program(CSBridge_EXECUTABLE NAMES CSBridge.exe PATHS ${CMAKE_SOURCE_DIR}/tool/bin/
)


message(STATUS "CSBridge executable found at: ${CSBridge_EXECUTABLE}")
