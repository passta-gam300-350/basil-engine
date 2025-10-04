include(FetchContent)

function(suppress_dep_warnings)
    foreach(tgt IN LISTS ARGN)
        if(NOT TARGET ${tgt})
            message(WARNING "suppress_warnings: '${tgt}' is not a known target")
            continue()
        endif()

        # Detect the target type
        get_target_property(_type ${tgt} TYPE)

        if(_type STREQUAL "INTERFACE_LIBRARY")
            # INTERFACE targets don't compile, so only INTERFACE options are allowed
            target_compile_options(${tgt} INTERFACE
                $<$<CXX_COMPILER_ID:MSVC>:/W0>
                $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-w>
            )
        elseif(_type STREQUAL "UTILITY" OR _type STREQUAL "UNKNOWN_LIBRARY")
            # UTILITY targets (custom commands) don't compile at all
            message(STATUS "suppress_warnings: '${tgt}' is not a compilable target (type=${_type}), skipping")
        else()
            # Normal compilable targets (STATIC/SHARED/OBJECT/EXECUTABLE)
            target_compile_options(${tgt} PRIVATE
                $<$<CXX_COMPILER_ID:MSVC>:/W0>
                $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-w>
            )
        endif()
    endforeach()
endfunction()


macro(import_catch)
    FetchContent_Declare(
        catch
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.9.1
    )

    FetchContent_MakeAvailable(catch)
endmacro()

macro(import_gtest)
    FetchContent_Declare(
        gtest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
    )

    FetchContent_MakeAvailable(gtest)
endmacro()

macro(import_spdlog)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.15.3
    )

    FetchContent_MakeAvailable(spdlog)
endmacro()

macro(import_glad)
    set(GLAD_DIR ${CMAKE_SOURCE_DIR}/dep/vendor/glad)
    set(GLAD_SRC_FILES ${GLAD_DIR}/src/glad.c)
    add_library(glad STATIC
        ${GLAD_SRC_FILES}
    )
    target_include_directories(glad PUBLIC ${GLAD_DIR}/include)
endmacro()

macro(import_glfw)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
    )

    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(glfw)
endmacro()

macro(import_mesh_optimiser)
    FetchContent_Declare(
      mesh_optimiser
      GIT_REPOSITORY https://github.com/zeux/meshoptimizer.git
      GIT_TAG v0.25
    )

    FetchContent_MakeAvailable(mesh_optimiser)
endmacro()

macro(import_glm)
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG release-1.0.2
    )

    FetchContent_MakeAvailable(glm)
endmacro()

macro(import_assimp)
    FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v6.0.2
    )

    FetchContent_MakeAvailable(assimp)
endmacro()

macro(import_entt)
    FetchContent_Declare(
        entt
        GIT_REPOSITORY https://github.com/skypjack/entt.git
        GIT_TAG v3.15.0
    )

    FetchContent_MakeAvailable(entt)
endmacro()

macro(import_zlib)
    FetchContent_Declare(
        zlib
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG v1.3.1
    )

    FetchContent_MakeAvailable(zlib)
endmacro()

macro(import_tinyddsloader)
    FetchContent_Declare(
        tinyddsloader
        GIT_REPOSITORY https://github.com/benikabocha/tinyddsloader.git
        GIT_TAG 49654013d03aaf38e527f0ae0e179d1a811e67b7
    )
    
    set(GLFW_INCLUDE_DIR "${glfw_SOURCE_DIR}/include" CACHE PATH "" FORCE)
    set(GLFW_LIBRARIES glfw CACHE STRING "" FORCE)

    FetchContent_MakeAvailable(tinyddsloader)
    add_library(tinyddsloader INTERFACE)
    target_sources(tinyddsloader INTERFACE "${tinyddsloader_SOURCE_DIR}/tinyddsloader.h")
    target_include_directories(tinyddsloader INTERFACE $<BUILD_INTERFACE:${tinyddsloader_SOURCE_DIR}>)
endmacro()

macro(import_yaml_cpp)
    FetchContent_Declare(
        yaml_cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG 0.8.0
    )

    FetchContent_MakeAvailable(yaml_cpp)
endmacro()

macro(import_imgui)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG docking
    )

    FetchContent_MakeAvailable(imgui)
    set(IMGUI_SOURCE
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui.cpp
    )

    set(IMGUI_MISC
        ${imgui_SOURCE_DIR}/misc/debuggers/imgui.natvis
        ${imgui_SOURCE_DIR}/misc/debuggers/imgui.natstepfilter
        ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.h
        ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
    )

    set(IMGUI_HEADER
        ${imgui_SOURCE_DIR}/imgui_internal.h
        ${imgui_SOURCE_DIR}/imconfig.h
        ${imgui_SOURCE_DIR}/imgui.h
        ${imgui_SOURCE_DIR}/imstb_rectpack.h
        ${imgui_SOURCE_DIR}/imstb_textedit.h
        ${imgui_SOURCE_DIR}/imstb_truetype.h
    )

    set(BACKENDS_HEADER
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.h
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.h
    )
    set(IMGUI_BACKENDS
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )

    file(GLOB FONTS_FILES ${imgui_SOURCE_DIR}/misc/fonts/*.ttf)

    set(IMGUI_FILES ${IMGUI_SOURCE} ${IMGUI_HEADER} ${IMGUI_MISC} ${FONTS_FILES})

    add_library(imgui STATIC
        ${IMGUI_FILES}
    )
    add_library(imgui_backends STATIC
        ${IMGUI_BACKENDS}
        ${BACKENDS_HEADER}
    )


    target_include_directories(imgui_backends PUBLIC ${imgui_SOURCE_DIR}/backends)
    target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
    target_link_libraries(imgui_backends PRIVATE glfw glad imgui)
    endmacro()

macro(import_stb)
    set(STB_DIR ${CMAKE_SOURCE_DIR}/dep/vendor/stb)
    set(STB_SRC_FILES
        ${STB_DIR}/stb_image.cpp
    )
    add_library(stb STATIC
        ${STB_SRC_FILES}
    )
    target_include_directories(stb PUBLIC ${STB_DIR})
endmacro()

macro(import_directxtex)
    FetchContent_Declare(
      directxtex
      GIT_REPOSITORY https://github.com/microsoft/DirectXTex.git
      GIT_TAG jul2025
    )

    FetchContent_MakeAvailable(directxtex)
endmacro()

# Import Mono
macro(import_mono)
    set(MONO_IMPORT_DIR "${CMAKE_SOURCE_DIR}/dep/vendor/mono")
    set(MONO_IMPORT_INCLUDE_DIR "${MONO_IMPORT_DIR}/include/mono-2.0")
    set(MONO_IMPORT_LIB_DIR "${MONO_IMPORT_DIR}/lib")
    set(MONO_IMPORT_SUCCESS TRUE)

    set(MONO_ASSEMBLY_PATH "${MONO_IMPORT_DIR}/lib/")
    set(MONO_CONFIG_PATH "${MONO_IMPORT_DIR}/etc/")

    if(NOT EXISTS "${MONO_IMPORT_DIR}")
        message(STATUS "MONO not found in MONO/ folder")
        set(MONO_IMPORT_SUCCESS FALSE)
    endif()

    if(NOT EXISTS "${MONO_IMPORT_INCLUDE_DIR}")
        message(STATUS "MONO include directory not found in mono/ folder")
        set(MONO_IMPORT_SUCCESS FALSE)
    endif()

    if(NOT EXISTS "${MONO_IMPORT_LIB_DIR}")
        message(STATUS "MONO lib directory not found in mono/ folder")
        set(MONO_IMPORT_SUCCESS FALSE)
    endif()

    IF(${MONO_IMPORT_SUCCESS})
        message(STATUS "${MONO_IMPORT_LIB_DIR} HERE")
        find_library(MONO_LIBRARY mono-2.0-sgen PATHS ${MONO_IMPORT_LIB_DIR})
        find_library(MONO_POSIX_LIBRARY MonoPosixHelper PATHS ${MONO_IMPORT_LIB_DIR})

        # Collect necessary bin files
        set(MONO_DLL_PATH "${MONO_IMPORT_DIR}/bin/mono-2.0-sgen.dll")
        set(MONO_DLL_STATIC "${MONO_IMPORT_DIR}/bin/libmono-btls-shared")
        set(MONO_DLL_POSIX "${MONO_IMPORT_DIR}/bin/MonoPosixHelper.dll")

        if(NOT MONO_LIBRARY)
            message(STATUS "MONO library not found in MONO/ folder")
            set(MONO_IMPORT_SUCCESS FALSE)
        endif()

        set(CMAKE_CSharp_COMPILER "${MONO_IMPORT_DIR}/bin/csc")
        set(MONO_BIN_PATH "${MONO_IMPORT_DIR}/bin/")
        add_library(mono SHARED IMPORTED)
        message(STATUS "Mono: ${MONO_LIBRARY}")
        set_target_properties(mono PROPERTIES
            IMPORTED_IMPLIB_RELEASE "${MONO_LIBRARY}"
            IMPORTED_IMPLIB_DEBUG "${MONO_LIBRARY}"
            IMPORTED_IMPLIB_MINSIZEREL "${MONO_LIBRARY}"
            IMPORTED_IMPLIB_RELWITHDEBINFO "${MONO_LIBRARY}"
            IMPORTED_IMPLIB_INSTALLER "${MONO_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES ${MONO_IMPORT_INCLUDE_DIR}
        )

        add_library(mono_interface INTERFACE)

        target_link_libraries(mono_interface
            INTERFACE mono
            ${MONO_POSIX_LIBRARY}
        )
    ELSE()
        message(STATUS "MONO Import Failed.")
    ENDIF()
endmacro()

macro(import_xml)
    FetchContent_Declare(
        pugixml
        GIT_REPOSITORY https://github.com/zeux/pugixml.git
        GIT_TAG v1.15
    )
    FetchContent_MakeAvailable(pugixml)
endmacro()

function(hide_dependencies)
# hide external targets to folders
    set_target_properties(
        glad 
        glfw
        uninstall
        update_mappings
        glm 
        assimp 
        EnTT 
        imgui 
        UpdateAssimpLibsDebugSymbolsAndDLLs 
        zlibstatic 
        Catch2
        Catch2WithMain
        ddsloader
        ddsview
        DirectXTex
        meshoptimizer
        pugixml-static
        spdlog
        stb
        texassemble
        texconv
        texdiag
        yaml-cpp
        yaml-cpp-parse
        yaml-cpp-read
        yaml-cpp-sandbox
        imgui_backends
        PROPERTIES FOLDER dep)
    suppress_dep_warnings(
        glad 
        glfw
        uninstall
        update_mappings
        glm 
        assimp 
        EnTT 
        imgui 
        UpdateAssimpLibsDebugSymbolsAndDLLs 
        zlibstatic 
        Catch2
        Catch2WithMain
        ddsloader
        ddsview
        DirectXTex
        meshoptimizer
        pugixml-static
        spdlog
        stb
        texassemble
        texconv
        texdiag
        yaml-cpp
        yaml-cpp-parse
        yaml-cpp-read
        yaml-cpp-sandbox
        imgui_backends)
endfunction()

# Macro to import all dependencies
macro(import_dependencies)
    if (NOT EXISTS "${CMAKE_SOURCE_DIR}/out/_dep")
        make_directory("${CMAKE_SOURCE_DIR}/out/_dep")
    endif()
    set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/out/_dep")
    import_glad()
    import_glfw()
    import_glm()
    import_entt()
    import_assimp()
    import_mesh_optimiser()
    import_tinyddsloader()
    import_directxtex()
    import_imgui()
    import_stb()

    import_spdlog()
    import_catch()
    import_yaml_cpp()
    #import_zlib()

    import_mono()
    import_xml()

    hide_dependencies()

    # Only set properties for mono_interface if it was created
    if(TARGET mono_interface)
        set_target_properties(mono_interface PROPERTIES FOLDER dep)
    endif()
endmacro()

macro(mono_postimport target)
    if(DEFINED MONO_DLL_PATH AND EXISTS "${MONO_DLL_PATH}")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MONO_DLL_PATH}" "$<TARGET_FILE_DIR:${target}>"
        )
    endif()

    if(NOT WIN32)
        if(DEFINED MONO_DLL_POSIX AND EXISTS "${MONO_DLL_POSIX}")
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MONO_DLL_POSIX}" "$<TARGET_FILE_DIR:${target}>"
            )
        endif()
    endif()

    target_compile_definitions(${target} PUBLIC MONO_ASSEMBLY_PATH="${MONO_ASSEMBLY_PATH}")
    target_compile_definitions(${target} PUBLIC MONO_CONFIG_PATH="${MONO_CONFIG_PATH}")
    target_compile_definitions(${target} PUBLIC MONO_PATH="${MONO_IMPORT_DIR}")
    target_compile_definitions(${target} PUBLIC MONO_CSC_COMPILER_PATH="${CMAKE_CSharp_COMPILER}")
    target_compile_definitions(${target} PUBLIC MONO_BIN_PATH="${MONO_BIN_PATH}")
    target_compile_definitions(${target} PUBLIC MONO_COMPILER="${TOOL_DLL_CSCOMPILER}")
endmacro(mono_postimport target)

macro(postimport_dll targetName)
    mono_postimport(${targetName})
endmacro()