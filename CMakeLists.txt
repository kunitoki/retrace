# =============================================================================
# RETRACE - CMakeLists.txt
# =============================================================================
# Retrace is a spectral frequency modulated synthesiser built with the YUP
# framework. This project builds it as an audio plugin (CLAP, VST3, AU, AUv3,
# LV2, AAX) and as a standalone application, from a single shared core.
# =============================================================================

cmake_minimum_required (VERSION 3.31)

# =============================================================================
# PROJECT UTILITIES
# =============================================================================
# Shared helper macros (version parsing, Apple deployment target detection).
include (${CMAKE_CURRENT_SOURCE_DIR}/cmake/utilities.cmake)

# Read the version from the VERSION file for centralized version control. This
# sets CURRENT_VERSION, VERSION_MAJOR, VERSION_MINOR and VERSION_PATCH.
retrace_read_version ("${CMAKE_CURRENT_SOURCE_DIR}/VERSION")

# Detect the minimum Apple deployment target supported by the installed Xcode
# and set CMAKE_OSX_DEPLOYMENT_TARGET accordingly (no-op on other hosts).
retrace_setup_deployment_target ()

# =============================================================================
# PRODUCT CONFIGURATION
# =============================================================================
# Every value below must stay a plain literal string: the justfile, the CI
# workflows and the packaging scripts parse this block to stay in sync with
# CMake, and they cannot expand ${variable} references.

# The internal name of the project and the prefix of every target (no spaces).
# It must differ from PRODUCT_NAME: the target that builds every plugin format
# is named after it and the standalone application after the product, and two
# build outputs of the same name do not generate with single-config Ninja.
set (PROJECT_NAME "retrace")

# The display name of the product: the plugin as hosts list it, the standalone
# application, the installers and the archives.
set (PRODUCT_NAME "Retrace")

# Short descriptive text for the product.
set (PRODUCT_DESCRIPTION "A spectral frequency modulated synthesiser.")

# Company / author name, used for attribution, bundle ids and code signing. It
# is also the plugin vendor and the folder the user settings and presets live in.
set (COMPANY_NAME "YUP Audio")

# Identifier of the product (reverse domain notation). It is the plugin id of
# every format and the prefix of every bundle id, so it must stay stable.
set (BUNDLE_ID "audio.yup.retrace")

# Website / support URL.
set (PRODUCT_URL "https://github.com/kunitoki/retrace")

# Contact e-mail embedded in the plugin metadata.
set (PRODUCT_EMAIL "kunitoki@gmail.com")

# Product icon file (path to .png, with an .ico of the same name beside it).
set (APPLICATION_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/app-icon.png")

# Four character codes. These are case sensitive and must stay stable once shipped.
set (PLUGIN_CODE "Rtrc")              # Unique plugin id (AU subtype / AAX product id)
set (PLUGIN_MANUFACTURER_CODE "Yup!") # Manufacturer id (exactly 4 characters, one uppercase)

# AAX plugin ids (native and AudioSuite) and category.
set (PLUGIN_AAX_NATIVE_CODE "RtrN")
set (PLUGIN_AAX_AUDIOSUITE_CODE "RtrA")
set (PLUGIN_AAX_CATEGORY "AAX_ePlugInCategory_SWGenerators")

# Unique id of the Windows installer (Inno Setup AppId). Keep it stable across
# releases so upgrades replace the previous installation.
set (INSTALLER_GUID "B1CB948E-FBD9-40B2-8196-6CD1307D82BC")

# Version can be overridden via command line: cmake -DTARGET_VERSION="2.0.0" ..
set (TARGET_VERSION "${CURRENT_VERSION}" CACHE STRING "Product version")

# =============================================================================
# BUILD OPTIONS & FEATURES
# =============================================================================

option (BUILD_TESTS "Build test suite" ON)
option (BUILD_BENCHMARKS "Build the audio benchmarks (google/benchmark)" ON)
option (ENABLE_COVERAGE "Enable code coverage analysis (requires lcov/gcov)" OFF)
option (ENABLE_CODE_SIGNING "Enable code signing (macOS/iOS)" OFF)

# Plugin formats to build. Toggle these to match your distribution targets.
option (PLUGIN_BUILD_CLAP "Build the CLAP format" ON)
option (PLUGIN_BUILD_VST3 "Build the VST3 format" ON)
option (PLUGIN_BUILD_AU "Build the Audio Unit format (macOS)" ON)
option (PLUGIN_BUILD_AUV3 "Build the Audio Unit v3 app extension" ON)
option (PLUGIN_BUILD_LV2 "Build the LV2 format" ON)
option (PLUGIN_BUILD_STANDALONE "Build the Standalone application of the plugin" ON)

# On macOS every build installs the plugins into the user plugin folders, so a
# host picks them up right away. Turn it off for CI and packaging builds.
option (PLUGIN_COPY_AFTER_BUILD "Install the plugins into the user plugin folders after each build (macOS)" ON)

# Code signing configuration (macOS/iOS).
set (MACOS_SIGNING_IDENTITY "" CACHE STRING "macOS code signing identity")
set (DEVELOPMENT_TEAM_ID "" CACHE STRING "Apple Developer Team ID")

# =============================================================================
# PROJECT DECLARATION
# =============================================================================

set (_retrace_project_languages C CXX)
if (CMAKE_HOST_APPLE)
  list (APPEND _retrace_project_languages OBJC OBJCXX)
endif()

project (${PROJECT_NAME} VERSION ${TARGET_VERSION} LANGUAGES ${_retrace_project_languages})

# The custom target that builds every enabled plugin format; the format targets
# are named "<PLUGIN_TARGET>_<format>_plugin" and the core "<PLUGIN_TARGET>_shared".
set (PLUGIN_TARGET "${PROJECT_NAME}")

# =============================================================================
# YUP FRAMEWORK SETUP
# =============================================================================

set (YUP_BUILD_EXAMPLES OFF)
set (YUP_BUILD_PLUGINS OFF)
set (YUP_BUILD_TESTS OFF)
set (YUP_ENABLE_VST3_VALIDATOR ON)
set (YUP_ENABLE_CLAP_VALIDATOR ON)
set (YUP_ENABLE_AUVAL_VALIDATOR ON)
set (YUP_ENABLE_PLUGINVAL ON)
add_subdirectory (yup)

# =============================================================================
# EMBEDDED RESOURCES
# =============================================================================
# The shaders and the factory presets are compiled into the plugin, so it has
# no files to install beside it.

set (PLUGIN_RESOURCE_TARGETS "")

# ==== The graphics shaders (data/shaders)
macro (retrace_add_shader_bundle suffix fragment_shader output_name resource_name)
    yup_add_shader_bundle (${PLUGIN_TARGET}_${suffix}
        VERT "${CMAKE_CURRENT_SOURCE_DIR}/data/shaders/fullscreen.vert"
        FRAG "${CMAKE_CURRENT_SOURCE_DIR}/data/shaders/${fragment_shader}"
        NAMESPACE retrace
        OUTPUT_NAME ${output_name}
        RESOURCE_NAME ${resource_name})

    list (APPEND PLUGIN_RESOURCE_TARGETS ${PLUGIN_TARGET}_${suffix})
endmacro()

retrace_add_shader_bundle (shaders                synth_waveform.frag          SynthWaveformShaderBundle      synthWaveformShader)
retrace_add_shader_bundle (glow_prefilter_shaders retrace_glow_prefilter.frag  SynthGlowPrefilterShaderBundle synthGlowPrefilterShader)
retrace_add_shader_bundle (blur_shaders           retrace_blur.frag            SynthBlurShaderBundle          synthBlurShader)
retrace_add_shader_bundle (neon_composite_shaders retrace_neon_composite.frag  SynthNeonCompositeShaderBundle synthNeonCompositeShader)

# ==== The factory presets (data/presets), packed into a bank: a zip archive
# embedded in the plugin as retrace::factoryBank_data / _size
set (factory_presets_dir "${CMAKE_CURRENT_SOURCE_DIR}/data/presets")
set (factory_bank "${CMAKE_CURRENT_BINARY_DIR}/Factory.rebank")

file (GLOB_RECURSE factory_presets RELATIVE "${factory_presets_dir}" CONFIGURE_DEPENDS "${factory_presets_dir}/*.retrace")
list (SORT factory_presets)
list (TRANSFORM factory_presets PREPEND "${factory_presets_dir}/" OUTPUT_VARIABLE factory_preset_files)
set_property (DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${factory_preset_files})

# A fixed time stamp keeps the archive, and so the embedded bytes, the same until a preset changes
execute_process (
    COMMAND ${CMAKE_COMMAND} -E tar cf "${factory_bank}" --format=zip --mtime=2026-01-01 -- ${factory_presets}
    WORKING_DIRECTORY "${factory_presets_dir}"
    RESULT_VARIABLE factory_bank_result)

if (NOT factory_bank_result EQUAL 0)
    message (FATAL_ERROR "Cannot pack the factory presets into ${factory_bank}")
endif()

yup_add_embedded_binary_resources (${PLUGIN_TARGET}_factory_bank
    OUT_DIR FactoryBank
    HEADER FactoryBank.h
    NAMESPACE retrace
    RESOURCE_NAMES factoryBank
    RESOURCES "${factory_bank}")

list (APPEND PLUGIN_RESOURCE_TARGETS ${PLUGIN_TARGET}_factory_bank)

# =============================================================================
# SOURCE FILES
# =============================================================================
# source/         - the audio processor and its editor
# source/dsp      - the sound engine, the parameters and the patch format
# source/ui       - the interface
# source/browser  - the preset library and browser model
# source/mcp      - the MCP server and the audio tools it offers

file (GLOB_RECURSE PLUGIN_SOURCES
  CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h"
)

source_group (TREE ${CMAKE_CURRENT_SOURCE_DIR}/source PREFIX "Source Files" FILES ${PLUGIN_SOURCES})

# =============================================================================
# AUDIO PLUGIN TARGET
# =============================================================================

# ==== AAX (optional, auto-detected)
# AAX requires the Avid SDK. It is enabled when YUP_AAX_SDK_ROOT is set (env or
# cache), or when the SDK sits in an "AAX" folder beside this repository.
set (plugin_create_aax OFF)
if (DEFINED YUP_AAX_SDK_ROOT OR DEFINED ENV{YUP_AAX_SDK_ROOT})
    if (DEFINED ENV{YUP_AAX_SDK_ROOT} AND NOT DEFINED YUP_AAX_SDK_ROOT)
        set (YUP_AAX_SDK_ROOT "$ENV{YUP_AAX_SDK_ROOT}" CACHE PATH "Path to the AAX SDK")
    endif()
    message (STATUS "AAX SDK using YUP_AAX_SDK_ROOT: ${YUP_AAX_SDK_ROOT}")
    set (plugin_create_aax ON)
else()
    set (aax_sdk_candidate "${CMAKE_CURRENT_SOURCE_DIR}/../AAX/aax-sdk-2-9-0")
    if (EXISTS "${aax_sdk_candidate}/Interfaces/AAX.h")
        set (YUP_AAX_SDK_ROOT "${aax_sdk_candidate}" CACHE PATH "Path to the AAX SDK")
        set (plugin_create_aax ON)
        message (STATUS "AAX SDK auto-detected at ${aax_sdk_candidate}")
    else()
        message (STATUS "AAX SDK not found - AAX target disabled")
    endif()
endif()

yup_audio_plugin (
    TARGET_NAME ${PLUGIN_TARGET}
    TARGET_VERSION ${TARGET_VERSION}
    TARGET_APP_ID "${BUNDLE_ID}"
    TARGET_APP_NAMESPACE "${BUNDLE_ID}"
    TARGET_CXX_STANDARD 20
    TARGET_IDE_GROUP "Retrace"

    PLUGIN_ID "${BUNDLE_ID}"
    PLUGIN_NAME "${PRODUCT_NAME}"
    PLUGIN_VENDOR "${COMPANY_NAME}"
    PLUGIN_EMAIL "${PRODUCT_EMAIL}"
    PLUGIN_VERSION "${TARGET_VERSION}"
    PLUGIN_DESCRIPTION "${PRODUCT_DESCRIPTION}"
    PLUGIN_URL "${PRODUCT_URL}"
    PLUGIN_ICON_BIG "${APPLICATION_ICON}"

    PLUGIN_IS_SYNTH ON
    PLUGIN_IS_MONO OFF

    PLUGIN_CREATE_CLAP ${PLUGIN_BUILD_CLAP}
    PLUGIN_CREATE_VST3 ${PLUGIN_BUILD_VST3}
    PLUGIN_CREATE_AU ${PLUGIN_BUILD_AU}
    PLUGIN_CREATE_AUv3 ${PLUGIN_BUILD_AUV3}
    PLUGIN_CREATE_LV2 ${PLUGIN_BUILD_LV2}
    PLUGIN_CREATE_STANDALONE ${PLUGIN_BUILD_STANDALONE}
    PLUGIN_CREATE_AAX ${plugin_create_aax}

    PLUGIN_AU_MANUFACTURER "${PLUGIN_MANUFACTURER_CODE}"
    PLUGIN_AU_SUBTYPE "${PLUGIN_CODE}"
    PLUGIN_AAX_MANUFACTURER_ID "${PLUGIN_MANUFACTURER_CODE}"
    PLUGIN_AAX_PRODUCT_ID "${PLUGIN_CODE}"
    PLUGIN_AAX_PLUGIN_ID_NATIVE "${PLUGIN_AAX_NATIVE_CODE}"
    PLUGIN_AAX_PLUGIN_ID_AUDIOSUITE "${PLUGIN_AAX_AUDIOSUITE_CODE}"
    PLUGIN_AAX_CATEGORY "${PLUGIN_AAX_CATEGORY}"

    PLUGIN_COPY_AFTER_BUILD ${PLUGIN_COPY_AFTER_BUILD}

    DEFINITIONS
      YUP_EMBED_DEFAULT_THEME_TEXT_SERIF_FONT=1

    MODULES
      yup::yup_gui
      yup::yup_audio_processors
      yup::yup_audio_gui
      yup::yup_ai
      yup::yup_audio_formats
      dr_libs
      libpng
      pffft_library
      ${PLUGIN_RESOURCE_TARGETS}
)

# The plugin sources compile once into the shared plugin core.
target_sources (${PLUGIN_TARGET}_shared PRIVATE ${PLUGIN_SOURCES})

# The shader bundles and the factory bank are OBJECT libraries, whose objects
# only reach the targets linking them directly: forward them with the core.
foreach (resource_target ${PLUGIN_RESOURCE_TARGETS})
    target_sources (${PLUGIN_TARGET}_shared INTERFACE $<TARGET_OBJECTS:${resource_target}>)
endforeach()

# ==== Link time optimization for the release builds of every plugin format
include (CheckIPOSupported)
check_ipo_supported (RESULT ipo_supported OUTPUT ipo_output)
if (ipo_supported)
    foreach (plugin_format clap vst3 standalone au auv3 aax lv2)
        if (TARGET ${PLUGIN_TARGET}_${plugin_format}_plugin)
            set_target_properties (${PLUGIN_TARGET}_${plugin_format}_plugin PROPERTIES
                INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        endif()
    endforeach()
else()
    message (STATUS "LTO not supported for ${PLUGIN_TARGET}: ${ipo_output}")
endif()

# ==== Coverage instruments the core, which the test suite links
if (ENABLE_COVERAGE AND (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"))
    target_compile_options (${PLUGIN_TARGET}_shared PRIVATE --coverage -fprofile-update=atomic)
    target_link_options (${PLUGIN_TARGET}_shared INTERFACE --coverage)
endif()

# =============================================================================
# CODE SIGNING (macOS)
# =============================================================================

if (APPLE AND ENABLE_CODE_SIGNING AND MACOS_SIGNING_IDENTITY)
    foreach (plugin_format clap vst3 au auv3 lv2 standalone aax)
        set (signable_target ${PLUGIN_TARGET}_${plugin_format}_plugin)
        if (TARGET ${signable_target})
            set_target_properties (${signable_target} PROPERTIES
                XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${MACOS_SIGNING_IDENTITY}"
                XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${DEVELOPMENT_TEAM_ID}"
                XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual")
        endif()
    endforeach()
    message (STATUS "macOS Code Signing: ${MACOS_SIGNING_IDENTITY}")
endif()

# =============================================================================
# TESTING
# =============================================================================
# The tests and the benchmarks link the plugin core, which only exists with
# every module in it on desktop.

if (BUILD_TESTS AND YUP_PLATFORM_DESKTOP)
    enable_testing()
    add_subdirectory (tests)
    message (STATUS "Building test suite")
endif()

if (BUILD_BENCHMARKS AND YUP_PLATFORM_DESKTOP)
    add_subdirectory (benchmarks)
    message (STATUS "Building audio benchmarks")
endif()
