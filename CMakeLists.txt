cmake_minimum_required(VERSION 3.20)
project(MediEvil_II_Recomp LANGUAGES C CXX)

set(PSXRECOMP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/psxrecomp" CACHE PATH
    "Path to psxrecomp")

# --- Opt-in product surfaces (OFF by default in the framework) -------------
# Scaffold prompts / flags fill these. Without recomp-ui, PSX_RECOMP_UI is OFF.
# Setup-host CI requires WIZARD_CMAKE_BLOCK / ENABLE_SETUP_WIZARD (FORCE_SETUP_HOST
# alone does not open first-run / Generate & rebuild).
# recomp-ui submodule present (PSX_RECOMP_UI defaults ON).
# Full netplay UI — enable after adding recomp-ui + testing:
# if(EXISTS "${PSXRECOMP_ROOT}/lib/recomp-net/CMakeLists.txt")
#     set(PSX_NETPLAY ON CACHE BOOL "…" FORCE)
# endif()
option(PSX_SETUP_WIZARD "Enable the developer code-generation wizard" OFF)
if(NOT PSX_SETUP_WIZARD)
    # Player builds always use the bundled backend. Linking a retail backend
    # would let first-run discovery silently select a BIOS elsewhere on disk.
    set(PSXRECOMP_BIOS_STEMS "OpenBIOS" CACHE STRING
        "BIOS backends included in the player package" FORCE)
endif()
include("${PSXRECOMP_ROOT}/runtime/runtime.cmake")

set(MEDIEVIL2_CODEGEN_SETUP_SOURCES)
if(PSX_SETUP_WIZARD)
    list(APPEND MEDIEVIL2_CODEGEN_SETUP_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/codegen_setup.c")
endif()

psxrecomp_add_game_runtime(psx-runtime
    PGXP
    # ENABLE_NETPLAY_IF_PRESENT
    # NETPLAY_LOBBY_URL "ws://netplay.retcomm.net:8765"
    WINDOW_TITLE "MediEvil II  Recompiled"
    DEFAULT_GAME_CONFIG_PATH "game.toml"
    MAX_PLAYERS 1
    LAUNCHER_BOXART "${CMAKE_CURRENT_SOURCE_DIR}/launcher_assets/img/boxart.tga"
    APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/psxrecomp.ico"
    # Generated from the player's disc; absent from the source/setup kit.
    GAME_OVERLAY_STATIC_C "${CMAKE_CURRENT_SOURCE_DIR}/generated/overlays_static.c"
    GEN_MARKER "generated/SCUS_945.64_dispatch.c"
    GEN_FULL_GLOB "generated/SCUS_945.64_full_*.c"
    PRELOADED_MODS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/mods/preloaded"
    # Resident Loading owns this title's loading acceleration.
    EXCLUDE_BUILTIN_MODS
        psx.enhancement.cd-speed
        psx.enhancement.fast-loading
        # Temporarily unavailable while the HD pack is being investigated.
        psx.enhancement.hd-textures
    CODEGEN_SETUP_SOURCES ${MEDIEVIL2_CODEGEN_SETUP_SOURCES}
)

target_include_directories(psx-runtime PRIVATE
    "${PSXRECOMP_ROOT}/lib/recomp-net/include")
target_sources(psx-runtime PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/medievil2_texture_filter.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/medievil2_frame_rate.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/medievil2_widescreen.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/medievil2_terrain.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/medievil2_screen_layout.c")
target_sources(psx-runtime PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/medievil2_modern_controls.cpp")
# Also block stale/installed copies when extracting over an older release.
set_property(SOURCE "${PSXRECOMP_ROOT}/runtime/src/mod_runtime.cpp" APPEND PROPERTY
    COMPILE_DEFINITIONS "PSX_MOD_DISABLED_PACKAGES=\"psx.enhancement.hd-textures\"")
target_sources(psx-runtime PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/medievil2_seamless.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/medievil2_seamless_store.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/medievil2_pp20.cpp")

# Playtest diagnostics (mod package medievil2.diagnostics; default off only in
# public releases, see below). The
# build identity is part of every recording header, so a replay refuses a
# different build instead of reporting a false divergence.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" MEDIEVIL2_VERSION LIMIT_COUNT 1)
set(MEDIEVIL2_SOURCE_REV "nogit")
find_package(Git QUIET)
if(PSX_SOURCE_COMMIT)
    # Set by psxrecomp/tools/package_title_release.py, which also refuses a
    # binary lacking this identity. A WSL configure of a Windows worktree
    # cannot ask git itself.
    string(SUBSTRING "${PSX_SOURCE_COMMIT}" 0 12 MEDIEVIL2_SOURCE_REV)
elseif(GIT_FOUND)
    execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse --short=12 HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE _MEDIEVIL2_REV OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET RESULT_VARIABLE _MEDIEVIL2_REV_RESULT)
    if(_MEDIEVIL2_REV_RESULT EQUAL 0 AND _MEDIEVIL2_REV)
        set(MEDIEVIL2_SOURCE_REV "${_MEDIEVIL2_REV}")
    endif()
endif()
target_sources(psx-runtime PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/medievil2_diagnostics.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/medievil2_diagnostic_paths.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/medievil2_telemetry.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/medievil2_diagnostic_writer.h")
set_property(SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/medievil2_diagnostics.cpp" APPEND PROPERTY
    COMPILE_DEFINITIONS "MEDIEVIL2_DIAGNOSTIC_BUILD_ID=\"${MEDIEVIL2_VERSION}+${MEDIEVIL2_SOURCE_REV}\"")
target_include_directories(psx-runtime PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/mods")
# The source manifest says default off. Only public releases keep that
# (psxrecomp/tools/package_title_release.py --channel public); dev builds and
# playtest packages stage it default-on.
include("${PSXRECOMP_ROOT}/runtime/playtest_diagnostics.cmake")
psxrecomp_playtest_diagnostics_defaults(psx-runtime MOD_ID "medievil2.diagnostics")

# Host-only diagnostics tests (no disc, BIOS or window). Run with ctest.
option(MEDIEVIL2_BUILD_TESTS "Build the diagnostics unit tests" ON)
if(MEDIEVIL2_BUILD_TESTS)
    enable_testing()
    find_package(Threads REQUIRED)
    add_executable(medievil2_diagnostic_writer_test tests/test_medievil2_diagnostic_writer.cpp)
    add_executable(medievil2_diagnostics_test tests/test_medievil2_diagnostics.cpp)
    foreach(_T medievil2_diagnostic_writer_test medievil2_diagnostics_test)
        set_target_properties(${_T} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
        target_include_directories(${_T} PRIVATE src/mods "${PSXRECOMP_ROOT}/runtime/include")
        target_link_libraries(${_T} PRIVATE Threads::Threads)
    endforeach()
    add_test(NAME medievil2_diagnostic_writer_test COMMAND medievil2_diagnostic_writer_test
        "${CMAKE_CURRENT_BINARY_DIR}/diagnostic-writer-qa")
    set_tests_properties(medievil2_diagnostic_writer_test PROPERTIES TIMEOUT 15)
    add_test(NAME medievil2_diagnostics_test COMMAND medievil2_diagnostics_test
        "${CMAKE_CURRENT_BINARY_DIR}/diagnostics-qa")
    if(TARGET SDL3::SDL3 OR TARGET SDL2::SDL2)
        add_executable(medievil2_modern_controls_test tests/test_modern_controls.cpp)
        target_compile_features(medievil2_modern_controls_test PRIVATE cxx_std_17)
        target_include_directories(medievil2_modern_controls_test PRIVATE "${PSXRECOMP_ROOT}/runtime/include")
        target_compile_definitions(medievil2_modern_controls_test PRIVATE SDL_MAIN_HANDLED)
        if(PSX_SDL_BACKEND STREQUAL "SDL3")
            target_compile_definitions(medievil2_modern_controls_test PRIVATE PSX_SDL3=1)
            target_link_libraries(medievil2_modern_controls_test PRIVATE SDL3::SDL3)
        else()
            target_link_libraries(medievil2_modern_controls_test PRIVATE SDL2::SDL2)
        endif()
        add_test(NAME medievil2_modern_controls_test COMMAND medievil2_modern_controls_test)
    endif()
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_FOUND)
        add_test(NAME medievil2_diagnostic_replay_prepare_test
            COMMAND "${Python3_EXECUTABLE}" -I
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_replay_medievil2_diagnostics.py")
        add_test(NAME medievil2_feedback_analysis_test
            COMMAND "${Python3_EXECUTABLE}" -I
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_analyze_medievil2_feedback.py")
    endif()
endif()

# Stage runtime configs next to the exe for direct build/ launches. Only copy
# files that exist — never fail POST_BUILD when game_options.toml is absent.
foreach(_PSX_RUNTIME_CONFIG game.toml game_options.toml)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_PSX_RUNTIME_CONFIG}")
        add_custom_command(TARGET psx-runtime POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/${_PSX_RUNTIME_CONFIG}"
                "$<TARGET_FILE_DIR:psx-runtime>/${_PSX_RUNTIME_CONFIG}"
            COMMENT "Staging ${_PSX_RUNTIME_CONFIG}"
            VERBATIM)
    endif()
endforeach()
