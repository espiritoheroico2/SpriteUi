cmake_minimum_required(VERSION 3.21)
project(SpriteUI VERSION 0.6.0 LANGUAGES CXX)

option(SPRITEUI_BUILD_EXAMPLES "Build SpriteUI examples" ON)
option(SPRITEUI_BUILD_TESTS "Build SpriteUI tests" ON)
option(SPRITEUI_BUILD_BENCHMARKS "Build lightweight CPU benchmarks" OFF)
option(SPRITEUI_BUILD_SDL3_ADAPTER "Build the optional SDL3 window-input adapter" OFF)
option(SPRITEUI_BUILD_VISUALIZER "Build the interactive SDL3 CPU visualizer" OFF)
option(SPRITEUI_BUILD_DOCS "Build the Doxygen API reference" OFF)
option(SPRITEUI_ENABLE_WARNINGS "Enable strict compiler warnings" ON)

add_library(SpriteUI
    src/Input/InputRouter.cpp
    src/Input/MultiPointerRouter.cpp
    src/Input/SpatialNavigation.cpp
    src/Animation/ToonMotion.cpp
    src/Rendering/DrawList.cpp
    src/Rendering/NineSlice.cpp
    src/Layout/Layout.cpp
    src/Layout/LayoutTree.cpp
    src/Layout/LayoutBridge.cpp
    src/Text/DebugShaper.cpp
    src/Text/LineBreak.cpp
    src/Text/TextLayout.cpp
    src/Widgets/Widgets.cpp
    src/Widgets/VirtualList.cpp
    src/Widgets/Docking.cpp
    src/Widgets/Graphs.cpp
    src/Widgets/Popup.cpp
    src/Config/UIConfig.cpp
)
add_library(SpriteUi::SpriteUI ALIAS SpriteUI)

if(SPRITEUI_BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_GENERATE_TREEVIEW YES)
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_WARN_AS_ERROR NO)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${CMAKE_CURRENT_SOURCE_DIR}/docs/API_REFERENCE.md")
    set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/docs")
    doxygen_add_docs(spriteui_docs
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/API_REFERENCE.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/TOOLENGINE_INTEGRATION_GUIDE.md"
        COMMENT "Generating SpriteUI API reference"
    )
endif()

if(SPRITEUI_BUILD_SDL3_ADAPTER)
    find_package(SDL3 3.2 CONFIG REQUIRED COMPONENTS SDL3)

    add_library(SpriteUI_SDL3Input
        src/Platform/SDL3InputAdapter.cpp
        src/Platform/SDL3Window.cpp
    )
    add_library(SpriteUi::SDL3Input ALIAS SpriteUI_SDL3Input)
    set_target_properties(SpriteUI_SDL3Input PROPERTIES EXPORT_NAME SDL3Input)
    target_compile_features(SpriteUI_SDL3Input PUBLIC cxx_std_20)
    target_include_directories(SpriteUI_SDL3Input
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(SpriteUI_SDL3Input
        PUBLIC SpriteUi::SpriteUI SDL3::SDL3
    )
    if(SPRITEUI_ENABLE_WARNINGS)
        if(MSVC)
            target_compile_options(SpriteUI_SDL3Input PRIVATE /W4 /permissive-)
        else()
            target_compile_options(SpriteUI_SDL3Input PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
        endif()
    endif()
endif()

target_compile_features(SpriteUI PUBLIC cxx_std_20)
target_include_directories(SpriteUI
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(SPRITEUI_ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(SpriteUI PRIVATE /W4 /permissive-)
    else()
        target_compile_options(SpriteUI PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    endif()
endif()

if(SPRITEUI_BUILD_EXAMPLES)
    add_executable(spriteui_hello examples/hello_spriteui.cpp)
    target_link_libraries(spriteui_hello PRIVATE SpriteUi::SpriteUI)

    add_executable(spriteui_interaction_demo examples/interaction_demo.cpp)
    target_link_libraries(spriteui_interaction_demo PRIVATE SpriteUi::SpriteUI)
endif()

if(SPRITEUI_BUILD_VISUALIZER)
    if(NOT WIN32)
        message(FATAL_ERROR "The visualizer's TTF/OTF font rasterizer currently requires Windows GDI.")
    endif()
    find_package(SDL3 3.2 CONFIG REQUIRED COMPONENTS SDL3)

    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/Fonts/Xmind"
        DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Fonts")
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/fotos"
        DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

    add_executable(spriteui_visualizer
        examples/BitmapFont.cpp
        examples/BitmapImage.cpp
        examples/spriteui_visualizer.cpp
    )
    target_include_directories(spriteui_visualizer PRIVATE tests)
    target_link_libraries(spriteui_visualizer PRIVATE SpriteUi::SpriteUI SDL3::SDL3)
    if(WIN32)
        target_link_libraries(spriteui_visualizer PRIVATE gdi32 ole32 windowscodecs)
    endif()
    add_custom_command(TARGET spriteui_visualizer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/Fonts/Xmind"
            "$<TARGET_FILE_DIR:spriteui_visualizer>/Fonts/Xmind"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/fotos"
            "$<TARGET_FILE_DIR:spriteui_visualizer>/fotos"
        VERBATIM
    )
    if(SPRITEUI_ENABLE_WARNINGS)
        if(MSVC)
            target_compile_options(spriteui_visualizer PRIVATE /W4 /permissive-)
        else()
            target_compile_options(spriteui_visualizer PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
        endif()
    endif()
    if(WIN32)
        add_custom_command(TARGET spriteui_visualizer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:SDL3::SDL3>"
                "$<TARGET_FILE_DIR:spriteui_visualizer>"
            VERBATIM
        )
    endif()
    if(SPRITEUI_BUILD_TESTS)
        add_test(NAME SpriteUI.VisualizerSnapshot
            COMMAND spriteui_visualizer --headless)
        set_tests_properties(SpriteUI.VisualizerSnapshot PROPERTIES
            ENVIRONMENT "SPRITEUI_SNAPSHOT_DIR=${CMAKE_CURRENT_BINARY_DIR}/visualizer-snapshots")
    endif()
endif()

if(SPRITEUI_BUILD_TESTS)
    enable_testing()
    if(SPRITEUI_BUILD_SDL3_ADAPTER)
        add_executable(spriteui_sdl3_input_tests tests/sdl3_input_adapter_tests.cpp)
        target_link_libraries(spriteui_sdl3_input_tests PRIVATE SpriteUi::SDL3Input)
        if(WIN32)
            add_custom_command(TARGET spriteui_sdl3_input_tests POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:SDL3::SDL3>"
                    "$<TARGET_FILE_DIR:spriteui_sdl3_input_tests>"
                VERBATIM
            )
        endif()
        add_test(NAME SpriteUI.SDL3Input COMMAND spriteui_sdl3_input_tests)
        set_tests_properties(SpriteUI.SDL3Input PROPERTIES
            ENVIRONMENT "SDL_VIDEODRIVER=dummy")
        add_test(NAME SpriteUI.SDL3Clipboard
            COMMAND spriteui_sdl3_input_tests --clipboard)
        set_tests_properties(SpriteUI.SDL3Clipboard PROPERTIES
            ENVIRONMENT "SDL_VIDEODRIVER=dummy"
            SKIP_RETURN_CODE 77)
    endif()

    add_executable(spriteui_tests
        tests/test_main.cpp
        tests/test_graph_sampling.cpp
        tests/test_text_breaking.cpp
        tests/test_text_layout.cpp
        tests/test_flex_wrap.cpp
        tests/test_nine_slice.cpp
        tests/test_visual.cpp
        tests/test_robustness.cpp
        tests/Legacy/LegacyLayout.cpp
    )
    target_include_directories(spriteui_tests PRIVATE tests tests/Legacy)
    target_link_libraries(spriteui_tests PRIVATE SpriteUi::SpriteUI)
    # Visual tests write PNG screenshots here (also asserted on pixel values).
    add_test(NAME SpriteUI.CoreTests COMMAND spriteui_tests)
    set_tests_properties(SpriteUI.CoreTests PROPERTIES
        ENVIRONMENT "SPRITEUI_SNAPSHOT_DIR=${CMAKE_CURRENT_BINARY_DIR}/snapshots")

    add_test(NAME SpriteUI.PackageSmoke
        COMMAND ${CMAKE_COMMAND}
            "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
            "-DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/package-smoke"
            "-DPROJECT_BINARY_DIR=${CMAKE_BINARY_DIR}"
            "-DGENERATOR=${CMAKE_GENERATOR}"
            "-DDEPENDENCY_PREFIX_PATH=${CMAKE_PREFIX_PATH}"
            "-DPACKAGE_HAS_SDL3=${SPRITEUI_BUILD_SDL3_ADAPTER}"
            "-DGENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
            "-DGENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}"
            "-DCXX_COMPILER=${CMAKE_CXX_COMPILER}"
            "-DCXX_FLAGS=${CMAKE_CXX_FLAGS}"
            "-DEXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}"
            "-DCONFIG=$<CONFIG>"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/PackageSmokeTest.cmake"
    )
endif()

if(SPRITEUI_BUILD_BENCHMARKS)
    add_executable(spriteui_benchmarks benchmarks/spriteui_benchmarks.cpp)
    target_link_libraries(spriteui_benchmarks PRIVATE SpriteUi::SpriteUI)
endif()

include(GNUInstallDirs)
install(TARGETS SpriteUI
    EXPORT SpriteUITargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
if(SPRITEUI_BUILD_SDL3_ADAPTER)
    install(TARGETS SpriteUI_SDL3Input
        EXPORT SpriteUITargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    set(SPRITEUI_CONFIG_SDL3_DEPENDENCY
        "include(CMakeFindDependencyMacro)\nfind_dependency(SDL3 3.2 CONFIG COMPONENTS SDL3)")
else()
    set(SPRITEUI_CONFIG_SDL3_DEPENDENCY "")
endif()
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
set(SPRITEUI_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/SpriteUI")
install(EXPORT SpriteUITargets
    FILE SpriteUITargets.cmake
    NAMESPACE SpriteUi::
    DESTINATION ${SPRITEUI_INSTALL_CMAKEDIR}
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/SpriteUIConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/SpriteUIConfig.cmake"
    INSTALL_DESTINATION "${SPRITEUI_INSTALL_CMAKEDIR}"
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/SpriteUIConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/SpriteUIConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/SpriteUIConfigVersion.cmake"
    DESTINATION "${SPRITEUI_INSTALL_CMAKEDIR}"
)

# Optional offline shader compilation. The CPU library remains buildable without DXC.
find_program(SPRITEUI_DXC_EXECUTABLE NAMES dxc)
if(SPRITEUI_DXC_EXECUTABLE)
    set(SPRITEUI_SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    file(MAKE_DIRECTORY "${SPRITEUI_SHADER_OUTPUT_DIR}")
    add_custom_command(
        OUTPUT "${SPRITEUI_SHADER_OUTPUT_DIR}/SpriteUI.vs.dxil"
        COMMAND "${SPRITEUI_DXC_EXECUTABLE}" -T vs_6_0 -E VSMain
                "${CMAKE_CURRENT_SOURCE_DIR}/shaders/SpriteUI.hlsl"
                -Fo "${SPRITEUI_SHADER_OUTPUT_DIR}/SpriteUI.vs.dxil"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/SpriteUI.hlsl"
        VERBATIM
    )
    add_custom_command(
        OUTPUT "${SPRITEUI_SHADER_OUTPUT_DIR}/SpriteUI.ps.dxil"
        COMMAND "${SPRITEUI_DXC_EXECUTABLE}" -T ps_6_0 -E PSMain
                "${CMAKE_CURRENT_SOURCE_DIR}/shaders/SpriteUI.hlsl"
                -Fo "${SPRITEUI_SHADER_OUTPUT_DIR}/SpriteUI.ps.dxil"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/SpriteUI.hlsl"
        VERBATIM
    )
    add_custom_target(spriteui_compile_shaders ALL
        DEPENDS
            "${SPRITEUI_SHADER_OUTPUT_DIR}/SpriteUI.vs.dxil"
            "${SPRITEUI_SHADER_OUTPUT_DIR}/SpriteUI.ps.dxil"
    )
else()
    message(STATUS "DXC not found: HLSL compilation target is skipped")
endif()

# Padrao de projeto: todo header publico deve compilar sozinho (self-contained).
# Uso: cmake -DSPRITEUI_CHECK_HEADERS=ON ... && cmake --build . --target spriteui_check_headers
option(SPRITEUI_CHECK_HEADERS "Compile each public header as its own translation unit" OFF)
if(SPRITEUI_CHECK_HEADERS)
    file(GLOB_RECURSE SPRITEUI_PUBLIC_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/include"
         "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
    set(SPRITEUI_HEADER_CHECK_SOURCES "")
    foreach(header IN LISTS SPRITEUI_PUBLIC_HEADERS)
        string(MAKE_C_IDENTIFIER "${header}" header_id)
        set(tu "${CMAKE_CURRENT_BINARY_DIR}/header_checks/${header_id}.cpp")
        file(WRITE "${tu}" "#include \"${header}\"\n")
        list(APPEND SPRITEUI_HEADER_CHECK_SOURCES "${tu}")
    endforeach()
    add_library(spriteui_check_headers OBJECT ${SPRITEUI_HEADER_CHECK_SOURCES})
    target_link_libraries(spriteui_check_headers PRIVATE SpriteUi::SpriteUI)
endif()
