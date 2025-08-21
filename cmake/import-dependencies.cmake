include(FetchContent)

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
    # FetchContent_Declare(
    #   glad
    #   GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    #   GIT_TAG v2.0.8
    #   SOURCE_SUBDIR cmake
    # )

    # need python interpretor (need install) to generate glad on the fly
    # FetchContent_MakeAvailable(glad)
    # glad_add_library(glad STATIC API gl:core=4.6 LOCATION ${PROJECT_SOURCE_DIR}/vendor/glad-build/${TARGET})

    # using generated glad src based on gl:core=4.6 no ext (update versioning here)
    set(GLAD_DIR ${CMAKE_SOURCE_DIR}/dep/vendor/glad)
    set(GLAD_SRC_FILES
        ${GLAD_DIR}/src/gl.c
    )
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

    FetchContent_MakeAvailable(glfw)
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
	    GIT_REPOSITORY	https://github.com/assimp/assimp.git
	    GIT_TAG			v6.0.2
    )

    FetchContent_MakeAvailable(assimp)
endmacro()

macro(import_reflect)
    FetchContent_Declare(
        reflect
        GIT_REPOSITORY https://github.com/getml/reflect-cpp.git
        GIT_TAG v0.20.0
    )

    FetchContent_MakeAvailable(reflect)
endmacro()

macro(import_entt)
    FetchContent_Declare(
        entt
        GIT_REPOSITORY https://github.com/skypjack/entt.git
        GIT_TAG v3.15.0
    )

    FetchContent_MakeAvailable(entt)
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
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
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
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.h
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.h
    )

    file(GLOB FONTS_FILES ${imgui_SOURCE_DIR}/misc/fonts/*.ttf)

    set(IMGUI_FILES ${IMGUI_SOURCE} ${IMGUI_HEADER} ${IMGUI_MISC} ${FONTS_FILES})
 
    add_library(imgui STATIC
        ${IMGUI_FILES}
    )

    target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
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

# Macro to import all dependencies
macro(import_dependencies)
    import_glad()
    import_glfw()
    import_glm()
    import_entt()
    import_assimp()
    import_reflect()
    import_imgui()
    import_stb()

    import_spdlog()
    import_catch()

    set_target_properties(glad glfw glm assimp EnTT imgui UpdateAssimpLibsDebugSymbolsAndDLLs zlibstatic PROPERTIES FOLDER dep)
endmacro()