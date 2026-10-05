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
target_sources(psx-runtime PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/medievil2_seamless.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/medievil2_seamless_store.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/medievil2_pp20.cpp")

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
