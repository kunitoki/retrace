cmake_minimum_required(VERSION 3.31)

enable_testing()

# ==== Setup googletests
set (gtest_force_shared_crt ON CACHE BOOL "" FORCE)

_yup_fetchcontent_declare (googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0)

FetchContent_MakeAvailable (googletest)

set_target_properties (gtest PROPERTIES FOLDER "Tests")
set_target_properties (gtest_main PROPERTIES FOLDER "Tests")
set_target_properties (gmock PROPERTIES FOLDER "Tests")
set_target_properties (gmock_main PROPERTIES FOLDER "Tests")

# ==== Create executable
# The tests link the plugin core, like every plugin format does: the plugin
# sources, the YUP modules and the embedded resources compile once, and the
# tests see the same definitions (YupPlugin_Name, ...) the plugin is built with.
set (target_name ${PROJECT_NAME}_tests)

yup_standalone_app (
    TARGET_NAME ${target_name}
    TARGET_VERSION ${TARGET_VERSION}
    TARGET_IDE_GROUP "Tests"
    TARGET_APP_ID "${BUNDLE_ID}.${target_name}"
    TARGET_APP_NAMESPACE "${BUNDLE_ID}"
    TARGET_CONSOLE OFF
    MODULES
        ${PLUGIN_TARGET}_shared
        GTest::gtest
        GTest::gmock)

# ==== Setup test sources
file (GLOB test_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/test_*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/helpers/*.h")
list (APPEND test_sources "${CMAKE_CURRENT_LIST_DIR}/main.cpp")

target_include_directories (${target_name} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/../source")
target_sources (${target_name} PRIVATE ${test_sources})
source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/ FILES ${test_sources})

# ==== Reuse YUP's shared GoogleTest runner (adds the runner source + gtest link)
yup_add_test_runner (${target_name})

# ==== Setup target properties
set_target_properties (${target_name} PROPERTIES
    XCODE_SCHEME_ARGUMENTS "--gtest_filter=*"
    VS_DEBUGGER_COMMAND_ARGUMENTS "--gtest_filter=*")

# ==== Register with CTest
add_test (NAME ${target_name} COMMAND ${target_name} --gtest_filter=*)
