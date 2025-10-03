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
    target_link_libraries(${exe} PRIVATE ${lib})
    add_custom_command(TARGET ${exe} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${resource_dir}
                $<TARGET_FILE_DIR:${exe}>/${dest_relative}
    )
endfunction()

function(link_engine exe)
    set(lib engine)
    set(res_path "${CMAKE_SOURCE_DIR}/bin/assets/shaders")
    link_lib_resource(${exe} ${lib} ${res_path} "assets/shaders")
endfunction()

