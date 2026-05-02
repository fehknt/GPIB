cmake_minimum_required(VERSION 3.10)
project(GPIB_Toolkit CXX C)

# Ensure C++11 or higher
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output all binaries to the bin directory where pre-existing DLLs are located
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin")

# Enable RC (Windows Resource Compiler)
enable_language(RC)

# Add the include directories
include_directories(include .)

# External libs from root/bin
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(PROJECT_LIBS sh_api.lib w32sal.lib winvfx16.lib agvisa32.lib setupapi.lib)

# Suppress Microsoft CRT secure warnings and prevent min/max macro conflicts
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
add_compile_definitions(NOMINMAX)

# Create the gpib_common static library
file(GLOB COMMON_SOURCES "src/common/*.cpp")
# We'll filter out gpiblib.cpp from common since it's a separate library
list(FILTER COMMON_SOURCES EXCLUDE REGEX "src/common/gpiblib.cpp")
add_library(gpib_common STATIC ${COMMON_SOURCES} src/apps/satrace_utils.cpp src/apps/7470_synthesizers.cpp src/apps/7470_logic.cpp)

# Link essential Win32 system dependencies
target_link_libraries(gpib_common PUBLIC ws2_32 winmm comctl32)
target_compile_definitions(gpib_common PUBLIC BUILDING_GPIBLIB BUILDING_SPECAN)

# Create DLLs
add_library(gpiblib SHARED src/common/gpiblib.cpp)
target_compile_definitions(gpiblib PRIVATE BUILDING_GPIBLIB)
add_library(gnss SHARED src/apps/gnss.cpp)
add_library(specan SHARED src/apps/specan.cpp)

# Link DLLs
target_link_libraries(gpiblib PUBLIC gpib_common)
target_link_libraries(gnss gpib_common gpiblib ${PROJECT_LIBS})
target_link_libraries(specan gpib_common gpiblib ${PROJECT_LIBS})

# Automatically copy default ini files to the output directory if they don't exist
add_custom_command(TARGET gpiblib POST_BUILD
    COMMAND cmd.exe /c "if not exist \"$<TARGET_FILE_DIR:gpiblib>\\connect.ini\" copy /Y \"${CMAKE_SOURCE_DIR}\\default_connect.ini\" \"$<TARGET_FILE_DIR:gpiblib>\\connect.ini\" >nul"
    COMMAND cmd.exe /c "if not exist \"$<TARGET_FILE_DIR:gpiblib>\\7470.ini\" copy /Y \"${CMAKE_SOURCE_DIR}\\default_7470.ini\" \"$<TARGET_FILE_DIR:gpiblib>\\7470.ini\" >nul"
    COMMAND cmd.exe /c "if not exist \"$<TARGET_FILE_DIR:gpiblib>\\pn.ini\" copy /Y \"${CMAKE_SOURCE_DIR}\\default_pn.ini\" \"$<TARGET_FILE_DIR:gpiblib>\\pn.ini\" >nul"
)

# Graphical (GUI) Windows Applications
set(GUI_APPS "7470" "pn" "prologix" "ssm" "tcheck" "vna")
foreach(app IN LISTS GUI_APPS)
    add_executable(${app} WIN32 src/apps/${app}.cpp src/apps/${app}.rc)
    target_link_libraries(${app} gpib_common specan gnss gpiblib ${PROJECT_LIBS})
endforeach()

# Standard Windows Console Applications
file(GLOB ALL_APPS "src/apps/*.cpp")
foreach(app_file IN LISTS ALL_APPS)
    get_filename_component(app_name ${app_file} NAME_WE)

    # We ignore the ones we've already defined explicitly above
    if(NOT app_name STREQUAL "gnss" AND NOT app_name STREQUAL "specan" AND NOT app_name STREQUAL "satrace_utils" AND NOT app_name STREQUAL "gnss_utils" AND NOT app_name STREQUAL "7470_logic" AND NOT app_name STREQUAL "7470_synthesizers")
        list(FIND GUI_APPS "${app_name}" IS_GUI)
        if(IS_GUI EQUAL -1)
            add_executable(${app_name} "${app_file}")
            target_link_libraries(${app_name} gpib_common specan gnss gpiblib ${PROJECT_LIBS})
        endif()
    endif()
endforeach()

# Testing Support
enable_testing()

# Fetch and configure GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
# For Windows: prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(TEST_LIST "appfile" "7470lex" "spline" "sparams" "timeutil" "gfxfile" "ipconn" "renderer" "hpgl_render" "xy" "pn_math" "gpiblib" "satrace" "gnss" "gpib_utils" "7470_synthesizers" "7470_logic" "specan" "recorder" "vna_math")

foreach(test_name IN LISTS TEST_LIST)
    add_executable(test_${test_name} tests/test_${test_name}.cpp)
    if(test_name STREQUAL "hpgl_render" OR test_name STREQUAL "7470_logic" OR test_name STREQUAL "7470_synthesizers")
        target_link_libraries(test_${test_name} gpib_common GTest::gtest_main GTest::gmock ws2_32 winmm comctl32)
    else()
        target_link_libraries(test_${test_name} gpib_common specan gnss gpiblib GTest::gtest_main GTest::gmock ${PROJECT_LIBS})
    endif()
    
    # Isolate test binaries to build/tests
    set_target_properties(test_${test_name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/tests"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/tests"
    )

    include(GoogleTest)
    gtest_discover_tests(test_${test_name}
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:gpiblib>"
        PROPERTIES ENVIRONMENT "PATH=$<TARGET_FILE_DIR:gpiblib>;$ENV{PATH}"
    )
endforeach()
