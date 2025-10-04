# Define a macro to group source and header files by directory
macro(GROUP_FILES_BY_FOLDER all_files)
    foreach(FILE ${ALL_FILES}) 
        # Get the directory of the file
        get_filename_component(PARENT_DIR "${FILE}" PATH)

        # Replace '/' with '\' for Visual Studio group formatting
        string(REPLACE "/" "\\" GROUP "${PARENT_DIR}")

        # Classify files into Source or Header groups based on their extension
        if ("${FILE}" MATCHES ".*\\.cpp")
        set(GROUP "Source Files\\${GROUP}")
        elseif("${FILE}" MATCHES ".*\\.h")
        set(GROUP "Header Files\\${GROUP}")
        endif()

        # Add the file to the corresponding source group
        source_group("${GROUP}" FILES "${FILE}")
    endforeach()
endmacro()

function(link_lib_resource exe lib resource_dir dest_relative)

if(EXISTS "${resource_dir}")
    target_link_libraries(${exe} PRIVATE ${lib})
    add_custom_command(TARGET ${exe} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${resource_dir}
                $<TARGET_FILE_DIR:${exe}>/${dest_relative}
    )
else()
    message(WARNING "lib resource directory not found: ${resource_dir}")
endif()
    
endfunction()

function(group_targets_with_folders folder)
    foreach(tgt IN LISTS ARGN)
        if(TARGET ${tgt})
            set_target_properties(${tgt} PROPERTIES FOLDER ${folder})
        else()
            message(WARNING "Target '${tgt}' does not exist, skipping.")
        endif()
    endforeach()
endfunction()

function(group_unit_test_targets)
    foreach(tgt IN LISTS ARGN)
        if(TARGET ${tgt})
            set_target_properties(${tgt} PROPERTIES FOLDER "unit_test" CXX_CLANG_TIDY "")
        else()
            message(WARNING "Target '${tgt}' does not exist, skipping.")
        endif()
    endforeach()
endfunction()

function(group_engine_lib_targets)
    foreach(tgt IN LISTS ARGN)
        if(TARGET ${tgt})
            set_target_properties(${tgt} PROPERTIES FOLDER "engine_lib")
        else()
            message(WARNING "Target '${tgt}' does not exist, skipping.")
        endif()
    endforeach()
endfunction()

function(group_editor_lib_targets)
    foreach(tgt IN LISTS ARGN)
        if(TARGET ${tgt})
            set_target_properties(${tgt} PROPERTIES FOLDER "editor_lib")
        else()
            message(WARNING "Target '${tgt}' does not exist, skipping.")
        endif()
    endforeach()
endfunction()

function(group_lib_example_test_targets)
    foreach(tgt IN LISTS ARGN)
        if(TARGET ${tgt})
            set_target_properties(${tgt} PROPERTIES FOLDER "lib_example" CXX_CLANG_TIDY "")
        else()
            message(WARNING "Target '${tgt}' does not exist, skipping.")
        endif()
    endforeach()
endfunction()

function(link_engine exe)
    set(lib engine)
    set(res_path "${CMAKE_SOURCE_DIR}/bin/assets/shaders")
    link_lib_resource(${exe} ${lib} ${res_path} "assets/shaders")
endfunction()