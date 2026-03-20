cmake_minimum_required(VERSION 3.20)

project(PrefabSystemExample)

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib)

# ============================================================
# Minimal Test (ZERO dependencies - only tests UUID)
# ============================================================
add_executable(prefab_minimal_test
    minimal_test.cpp
)

# Only needs UUID headers - no engine, no ECS, no components
target_include_directories(prefab_minimal_test PRIVATE
    ${LIB_DIR}/uuid/include
)

target_compile_features(prefab_minimal_test PRIVATE cxx_std_20)

if(WIN32)
    target_compile_definitions(prefab_minimal_test PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
endif()

# Organize in IDE
set_target_properties(prefab_minimal_test PROPERTIES FOLDER lib_example)

# ============================================================
# Full Example (requires full engine with reflection)
# ============================================================
add_executable(prefab_example
    main.cpp
)

target_include_directories(prefab_example PRIVATE
    ${LIB_DIR}/uuid/include
)

target_link_libraries(prefab_example PRIVATE
    engine
)

# Set C++20 standard
target_compile_features(prefab_example PRIVATE cxx_std_20)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(prefab_example PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
endif()

# Organize in IDE
set_target_properties(prefab_example PROPERTIES
    FOLDER lib_example
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

# Copy sample prefab to build directory
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/sample_player.prefab
    ${CMAKE_CURRENT_BINARY_DIR}/sample_player.prefab
    COPYONLY
)
