project(resource_unit_test)

file(GLOB_RECURSE ALL_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_CURRENT_LIST_DIR}/*.[ch]pp
    ${CMAKE_CURRENT_LIST_DIR}/*.h
)

set(ALL_LIBS
  PRIVATE GTest::gtest
  PRIVATE resource_lib_ext
)

add_executable(${PROJECT_NAME} ${ALL_FILES})

target_link_libraries(${PROJECT_NAME} ${ALL_LIBS})


GROUP_UNIT_TEST_TARGETS(${PROJECT_NAME})