cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0091 NEW)
project(StellarisArgonUi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The static C runtime, as the launcher's plugin handbook recommends. Nothing owned by one plugin is freed by another: the interface of
# stellaris-guiexpand passes no ownership across the DLL boundary.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

include(FetchContent)

# Dear ImGui 1.85: the version compiled into stellaris.exe (4.5.2). The components draw with their own copy bound to the engine's context
# (stellaris_gui_imgui.hpp checks the layout at run time and refuses to draw on a mismatch).
# (offline or a local clone: -DFETCHCONTENT_SOURCE_DIR_IMGUI=<path to an ImGui v1.85 checkout>)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.85
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)
set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp)

add_library(stellaris_argon_ui SHARED
    src/argon.cpp
    src/draw.cpp
    src/components.cpp
    src/components_shell.cpp
    src/components_pages.cpp
    ${IMGUI_SOURCES})
target_include_directories(stellaris_argon_ui PRIVATE src include/stellaris_guiexpand ${imgui_SOURCE_DIR})
target_compile_definitions(stellaris_argon_ui PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
if (MSVC)
    target_compile_options(stellaris_argon_ui PRIVATE /utf-8)
endif()
set_target_properties(stellaris_argon_ui PROPERTIES PREFIX "")

# The plugin folder as the launcher expects it: stl-plugin.json and the DLL. Install it with `stl plugin install --link build\plugin\stellaris-argon-ui`.
set(ARGON_PLUGIN_DIR "${CMAKE_BINARY_DIR}/plugin/stellaris-argon-ui")
add_custom_target(stellaris_argon_ui_plugin ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ARGON_PLUGIN_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:stellaris_argon_ui>" "${ARGON_PLUGIN_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/plugin/stl-plugin.json" "${ARGON_PLUGIN_DIR}/"
    DEPENDS stellaris_argon_ui
    COMMENT "Assembling ${ARGON_PLUGIN_DIR}")
