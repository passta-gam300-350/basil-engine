project(job_system_lib CXX)

set(JOB_SYSTEM_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

file(GLOB_RECURSE JOB_SYSTEM_FILES
    ${JOB_SYSTEM_DIR}/src/*.[ch]pp
    ${JOB_SYSTEM_DIR}/src/*.h
)

add_library("${PROJECT_NAME}" INTERFACE)

target_include_directories("${PROJECT_NAME}" INTERFACE "${JOB_SYSTEM_DIR}/include")

target_link_libraries(${PROJECT_NAME} INTERFACE data_structure_lib)

target_compile_options(${PROJECT_NAME} INTERFACE
  $<$<CXX_COMPILER_ID:MSVC>:/W4>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

GROUP_ENGINE_LIB_TARGETS(${PROJECT_NAME})