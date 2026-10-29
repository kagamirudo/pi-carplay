cmake_minimum_required(VERSION 3.25)
project(raspi-headunit VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(Qt6 6.4 REQUIRED COMPONENTS Core DBus Gui Qml Quick)
qt_standard_project_setup()

add_library(headunit_core
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
target_link_libraries(headunit_core PUBLIC Threads::Threads)
target_compile_options(headunit_core PRIVATE -Wall -Wextra -Wpedantic)

qt_add_executable(headunit
    src/main.cpp
    src/core/ShellPresenter.cpp
    src/core/ShellPresenter.hpp
    src/bluetooth/BluetoothDevices.cpp
    src/bluetooth/BluetoothDevices.hpp
    src/bluetooth/BluetoothAgent.cpp
    src/bluetooth/BluetoothAgent.hpp
)
qt_add_qml_module(headunit
    URI Headunit
    VERSION 1.0
    QML_FILES
        ui/Main.qml
        ui/HomePage.qml
        ui/ConnectionPage.qml
        ui/DashcamPage.qml
        ui/SettingsPage.qml
        ui/BluetoothPage.qml
        ui/ActionButton.qml
)
target_link_libraries(headunit PRIVATE Qt6::Core Qt6::DBus Qt6::Gui Qt6::Qml Qt6::Quick headunit_core)
target_compile_definitions(headunit PRIVATE HEADUNIT_VERSION="${PROJECT_VERSION}")
target_compile_options(headunit PRIVATE -Wall -Wextra -Wpedantic)

enable_testing()
add_executable(headunit_tests
    tests/test_main.cpp
    tests/config_test.cpp
    tests/storage_test.cpp
    tests/phone_test.cpp
)
target_include_directories(headunit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
target_link_libraries(headunit_tests PRIVATE headunit_core)
target_compile_options(headunit_tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME headunit_tests COMMAND headunit_tests)

install(TARGETS headunit RUNTIME DESTINATION bin)
install(FILES config/headunit.json DESTINATION share/raspi-headunit)
install(FILES systemd/headunit.service DESTINATION share/raspi-headunit)
