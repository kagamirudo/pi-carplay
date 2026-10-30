cmake_minimum_required(VERSION 3.25)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/../VERSION" PI_CARPLAY_VERSION LIMIT_COUNT 1)
project(raspi-headunit VERSION ${PI_CARPLAY_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HEADUNIT_WERROR "Treat compiler warnings as errors" OFF)
set(HEADUNIT_SANITIZE "" CACHE STRING "Comma-separated -fsanitize= list, e.g. address,undefined")
option(HEADUNIT_COVERAGE "Instrument for gcov line and branch coverage" OFF)
option(HEADUNIT_TESTS "Build the test suite" ON)

find_package(Threads REQUIRED)
find_package(Qt6 6.4 REQUIRED COMPONENTS Core DBus Gui Qml Quick)
qt_standard_project_setup()

# Warnings, sanitizers and coverage apply to every target built here.
add_library(headunit_flags INTERFACE)
target_compile_options(headunit_flags INTERFACE -Wall -Wextra -Wpedantic)
if(HEADUNIT_WERROR)
    target_compile_options(headunit_flags INTERFACE -Werror)
endif()
if(HEADUNIT_SANITIZE)
    target_compile_options(headunit_flags INTERFACE -fsanitize=${HEADUNIT_SANITIZE} -fno-omit-frame-pointer
                                                    -fno-sanitize-recover=all)
    target_link_options(headunit_flags INTERFACE -fsanitize=${HEADUNIT_SANITIZE})
    target_compile_definitions(headunit_flags INTERFACE HEADUNIT_SANITIZED=1)
endif()
if(HEADUNIT_COVERAGE)
    target_compile_options(headunit_flags INTERFACE --coverage -O0)
    target_link_options(headunit_flags INTERFACE --coverage)
endif()

add_library(headunit_core STATIC
    src/core/Log.cpp
    src/core/ConfigurationManager.cpp
    src/core/ApplicationManager.cpp
    src/connection/PhoneConnectionManager.cpp
    src/connection/USBManager.cpp
    src/connection/WiFiManager.cpp
    src/bluetooth/BluetoothManager.cpp
    src/audio/AudioRouter.cpp
    src/projection/ProjectionManager.cpp
    src/projection/AndroidAutoReceiver.cpp
    src/projection/CarPlayReceiver.cpp
    src/video/VideoRenderer.cpp
    src/camera/DashcamRecorder.cpp
    src/camera/StorageManager.cpp
    src/input/TouchInputManager.cpp
)
target_include_directories(headunit_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(headunit_core PUBLIC Threads::Threads headunit_flags)

# Qt objects exposed to QML, shared by the executable and the tests.
add_library(headunit_shell STATIC
    src/core/ShellPresenter.cpp
    src/core/ShellPresenter.hpp
    src/bluetooth/BluetoothDevices.cpp
    src/bluetooth/BluetoothDevices.hpp
    src/bluetooth/BluetoothAgent.cpp
    src/bluetooth/BluetoothAgent.hpp
    src/bluetooth/DeviceList.cpp
    src/bluetooth/DeviceList.hpp
    src/connection/WifiDevices.cpp
    src/connection/WifiDevices.hpp
)
target_link_libraries(headunit_shell PUBLIC Qt6::Core Qt6::DBus headunit_core)
target_compile_definitions(headunit_shell PUBLIC HEADUNIT_VERSION="${PROJECT_VERSION}")

set(HEADUNIT_QML_FILES
    ui/Main.qml
    ui/HomePage.qml
    ui/ConnectionPage.qml
    ui/DashcamPage.qml
    ui/SettingsPage.qml
    ui/BluetoothPage.qml
    ui/WifiPage.qml
    ui/ActionButton.qml
)

qt_add_executable(headunit src/main.cpp)
qt_add_qml_module(headunit
    URI Headunit
    VERSION 1.0
    QML_FILES ${HEADUNIT_QML_FILES}
)
target_link_libraries(headunit PRIVATE Qt6::Gui Qt6::Qml Qt6::Quick headunit_shell)

if(HEADUNIT_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

install(TARGETS headunit RUNTIME DESTINATION bin)
install(FILES config/headunit.json DESTINATION share/raspi-headunit)
install(FILES systemd/headunit.service DESTINATION share/raspi-headunit)
