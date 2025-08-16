include(import-dependencies)

import_gtest()
enable_testing()

set_target_properties(unit gtest gtest_main gmock gmock_main PROPERTIES FOLDER dep)

add_subdirectory(dummy)
