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
            # target_compile_options(${tgt} INTERFACE
            #    $<$<CXX_COMPILER_ID:MSVC>:/W0>
            #    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-w>
            # )
        elseif(_type STREQUAL "UTILITY" OR _type STREQUAL "UNKNOWN_LIBRARY")
            # UTILITY targets (custom commands) don't compile at all
            message(STATUS "suppress_warnings: '${tgt}' is not a compilable target (type=${_type}), skipping")
        else()
            # Normal compilable targets (STATIC/SHARED/OBJECT/EXECUTABLE)
            # commented out. this causes the d9025 warnings when overriding the dependencies's cmake
            # target_compile_options(${tgt} PRIVATE
            #    $<$<CXX_COMPILER_ID:MSVC>:/W0>
            #    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-w>
            # )
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

    # Mark includes as SYSTEM to exclude from static analysis
    foreach(target Catch2 Catch2WithMain)
        if(TARGET ${target})
            get_target_property(inc_dirs ${target} INTERFACE_INCLUDE_DIRECTORIES)
            if(inc_dirs)
                set_target_properties(${target} PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${inc_dirs}")
            endif()
        endif()
    endforeach()
endmacro()

macro(import_gtest)
    FetchContent_Declare(
        gtest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
    )

    FetchContent_MakeAvailable(gtest)

    # Mark includes as SYSTEM to exclude from static analysis
    foreach(target gtest gtest_main gmock gmock_main)
        if(TARGET ${target})
            get_target_property(inc_dirs ${target} INTERFACE_INCLUDE_DIRECTORIES)
            if(inc_dirs)
                set_target_properties(${target} PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${inc_dirs}")
            endif()
        endif()
    endforeach()
endmacro()

macro(import_spdlog)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.15.3
    )

    FetchContent_MakeAvailable(spdlog)

    # Mark spdlog includes as SYSTEM to exclude from static analysis
    get_target_property(spdlog_include_dirs spdlog INTERFACE_INCLUDE_DIRECTORIES)
    set_target_properties(spdlog PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${spdlog_include_dirs}")
endmacro()

macro(import_glad)
    set(GLAD_DIR ${CMAKE_SOURCE_DIR}/dep/vendor/glad)
    set(GLAD_SRC_FILES ${GLAD_DIR}/src/glad.c)
    add_library(glad STATIC
        ${GLAD_SRC_FILES}
    )
    target_include_directories(glad SYSTEM PUBLIC ${GLAD_DIR}/include)
    set_target_properties(glad PROPERTIES CXX_CLANG_TIDY "")
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

    # Mark includes as SYSTEM to exclude from static analysis
    get_target_property(inc_dirs glfw INTERFACE_INCLUDE_DIRECTORIES)
    if(inc_dirs)
        set_target_properties(glfw PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${inc_dirs}")
    endif()
endmacro()

macro(import_mesh_optimiser)
    FetchContent_Declare(
      mesh_optimiser
      GIT_REPOSITORY https://github.com/zeux/meshoptimizer.git
      GIT_TAG v0.25
    )

    FetchContent_MakeAvailable(mesh_optimiser)

    # Mark includes as SYSTEM to exclude from static analysis
    get_target_property(inc_dirs meshoptimizer INTERFACE_INCLUDE_DIRECTORIES)
    if(inc_dirs)
        set_target_properties(meshoptimizer PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${inc_dirs}")
    endif()
endmacro()

macro(import_glm)
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.2
    )

    FetchContent_MakeAvailable(glm)

    # Mark includes as SYSTEM to exclude from static analysis
    get_target_property(inc_dirs glm INTERFACE_INCLUDE_DIRECTORIES)
    if(inc_dirs)
        set_target_properties(glm PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${inc_dirs}")
    endif()
endmacro()

macro(import_assimp)
    FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v6.0.2
    )

    FetchContent_MakeAvailable(assimp)

    # Mark includes as SYSTEM to exclude from static analysis
    get_target_property(inc_dirs assimp INTERFACE_INCLUDE_DIRECTORIES)
    if(inc_dirs)
        set_target_properties(assimp PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${inc_dirs}")
    endif()
endmacro()

macro(import_entt)
    FetchContent_Declare(
        entt
        GIT_REPOSITORY https://github.com/skypjack/entt.git
        GIT_TAG v3.15.0
    )

    FetchContent_MakeAvailable(entt)

    # Mark includes as SYSTEM to exclude from static analysis
    get_target_property(inc_dirs EnTT INTERFACE_INCLUDE_DIRECTORIES)
    if(inc_dirs)
        set_target_properties(EnTT PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${inc_dirs}")
    endif()
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
    set_target_properties(tinyddsloader PROPERTIES CXX_CLANG_TIDY "")
endmacro()

macro(import_yaml_cpp)
    FetchContent_Declare(
        yaml_cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG 0.8.0
    )

    FetchContent_MakeAvailable(yaml_cpp)

    # Mark includes as SYSTEM to exclude from static analysis
    get_target_property(inc_dirs yaml-cpp INTERFACE_INCLUDE_DIRECTORIES)
    if(inc_dirs)
        set_target_properties(yaml-cpp PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${inc_dirs}")
    endif()
endmacro()

macro(import_jolt)
    FetchContent_Declare(
        jolt
        GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
        GIT_TAG v5.4.0
        SOURCE_SUBDIR Build
    )

    FetchContent_MakeAvailable(jolt)

    set_target_properties(Jolt PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )

    # Mark includes as SYSTEM to exclude from static analysis
    get_target_property(inc_dirs Jolt INTERFACE_INCLUDE_DIRECTORIES)
    if(inc_dirs)
        set_target_properties(Jolt PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${inc_dirs}")
    endif()
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


    target_include_directories(imgui_backends SYSTEM PUBLIC ${imgui_SOURCE_DIR}/backends)
    target_include_directories(imgui SYSTEM PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
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
    target_include_directories(stb SYSTEM PUBLIC ${STB_DIR})
endmacro()

macro(import_directxtex)
    FetchContent_Declare(
      directxtex
      GIT_REPOSITORY https://github.com/microsoft/DirectXTex.git
      GIT_TAG jul2025
    )

    FetchContent_MakeAvailable(directxtex)

    # Mark DirectXTex includes as SYSTEM to exclude from static analysis
    get_target_property(directxtex_include_dirs DirectXTex INTERFACE_INCLUDE_DIRECTORIES)
    set_target_properties(DirectXTex PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${directxtex_include_dirs}")
endmacro()

macro(import_freetype)
    FetchContent_Declare(
      freetype
      GIT_REPOSITORY https://github.com/freetype/freetype.git
      GIT_TAG VER-2-14-1
    )

    FetchContent_MakeAvailable(freetype)
    include_directories(${freetype_SOURCE_DIR}/inc)
endmacro()

macro(import_fmod)
    set(FMOD_API_DIR "${CMAKE_SOURCE_DIR}/dep/vendor/fmod")
    set(FMOD_CORE_API_LIB_DIR "${FMOD_API_DIR}/api/core/lib/x64")
    set(FMOD_CORE_API_INC_DIR "${FMOD_API_DIR}/api/core/inc")

    set(FMOD_STUDIO_API_LIB_DIR "${FMOD_API_DIR}/api/studio/lib/x64")
    set(FMOD_STUDIO_API_INC_DIR "${FMOD_API_DIR}/api/studio/inc")

    find_library(FMOD_CORE_LIBRARY_DEBUG fmodL_vc PATHS ${FMOD_CORE_API_LIB_DIR})
    find_library(FMOD_CORE_LIBRARY_RELEASE fmod_vc PATHS ${FMOD_CORE_API_LIB_DIR})

    find_library(FMOD_STUDIO_LIBRARY_DEBUG fmodstudioL_vc PATHS ${FMOD_STUDIO_API_LIB_DIR})
    find_library(FMOD_STUDIO_LIBRARY_RELEASE fmodstudio_vc PATHS ${FMOD_STUDIO_API_LIB_DIR})

    add_library(fmod SHARED IMPORTED)
    set_target_properties(fmod PROPERTIES
        IMPORTED_LOCATION_DEBUG ${FMOD_CORE_API_LIB_DIR}/fmodL.dll
        IMPORTED_IMPLIB_DEBUG ${FMOD_CORE_LIBRARY_DEBUG}
        IMPORTED_LOCATION_RELEASE ${FMOD_CORE_API_LIB_DIR}/fmod.dll
        IMPORTED_IMPLIB_RELEASE ${FMOD_CORE_LIBRARY_RELEASE}
        IMPORTED_LOCATION_MINSIZEREL ${FMOD_CORE_API_LIB_DIR}/fmod.dll
        IMPORTED_IMPLIB_MINSIZEREL ${FMOD_CORE_LIBRARY_RELEASE}
        IMPORTED_LOCATION_RELWITHDEBINFO ${FMOD_CORE_API_LIB_DIR}/fmod.dll
        IMPORTED_IMPLIB_RELWITHDEBINFO ${FMOD_CORE_LIBRARY_RELEASE}
        IMPORTED_LOCATION_GAMERELEASE ${FMOD_CORE_API_LIB_DIR}/fmod.dll
        IMPORTED_IMPLIB_GAMERELEASE ${FMOD_CORE_LIBRARY_RELEASE}
        INTERFACE_INCLUDE_DIRECTORIES ${FMOD_CORE_API_INC_DIR}
        )

    add_library(fmodstudio SHARED IMPORTED)
    set_target_properties(fmodstudio PROPERTIES
        IMPORTED_LOCATION_DEBUG ${FMOD_STUDIO_API_LIB_DIR}/fmodstudioL.dll
        IMPORTED_IMPLIB_DEBUG ${FMOD_STUDIO_LIBRARY_DEBUG}
        IMPORTED_LOCATION_RELEASE ${FMOD_STUDIO_API_LIB_DIR}/fmodstudio.dll
        IMPORTED_IMPLIB_RELEASE ${FMOD_STUDIO_LIBRARY_RELEASE}
        IMPORTED_LOCATION_MINSIZEREL ${FMOD_STUDIO_API_LIB_DIR}/fmodstudio.dll
        IMPORTED_IMPLIB_MINSIZEREL ${FMOD_STUDIO_LIBRARY_RELEASE}
        IMPORTED_LOCATION_RELWITHDEBINFO ${FMOD_STUDIO_API_LIB_DIR}/fmodstudio.dll
        IMPORTED_IMPLIB_RELWITHDEBINFO ${FMOD_STUDIO_LIBRARY_RELEASE}
        IMPORTED_LOCATION_GAMERELEASE ${FMOD_CORE_API_LIB_DIR}/fmodstudio.dll
        IMPORTED_IMPLIB_GAMERELEASE ${FMOD_STUDIO_LIBRARY_RELEASE}
        INTERFACE_INCLUDE_DIRECTORIES ${FMOD_STUDIO_API_INC_DIR}
        )
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
            IMPORTED_IMPLIB_DEBUG_STATIC_ANALYSIS "${MONO_LIBRARY}"
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

    # Mark includes as SYSTEM to exclude from static analysis
    get_target_property(inc_dirs pugixml-static INTERFACE_INCLUDE_DIRECTORIES)
    if(inc_dirs)
        set_target_properties(pugixml-static PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${inc_dirs}")
    endif()
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

    import_freetype()
    import_fmod()
    import_mono()
    import_xml()
    import_jolt()

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