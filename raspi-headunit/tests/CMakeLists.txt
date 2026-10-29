find_package(Qt6 REQUIRED COMPONENTS Test)
find_program(DBUS_RUN_SESSION dbus-run-session)

# headunit_test(<name> LABELS <labels...> [LIBS <libs...>] [SOURCES <extra...>] [DBUS])
# Builds tst_<name>.cpp into its own executable and registers it with ctest.
function(headunit_test name)
    cmake_parse_arguments(ARG "DBUS" "" "LABELS;LIBS;SOURCES" ${ARGN})
    add_executable(tst_${name} tst_${name}.cpp ${ARG_SOURCES})
    target_link_libraries(tst_${name} PRIVATE Qt6::Test headunit_shell ${ARG_LIBS})
    target_compile_definitions(tst_${name} PRIVATE
        HEADUNIT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
        HEADUNIT_BINARY_DIR="${PROJECT_BINARY_DIR}")
    set(command $<TARGET_FILE:tst_${name}>)
    if(ARG_DBUS AND DBUS_RUN_SESSION)
        # A private session bus, so tests never touch the desktop or system bus.
        set(command ${DBUS_RUN_SESSION} -- ${command})
    endif()
    add_test(NAME ${name} COMMAND ${command})
    set_tests_properties(${name} PROPERTIES
        LABELS "${ARG_LABELS}"
        TIMEOUT 120
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen:configfile=${CMAKE_CURRENT_SOURCE_DIR}/panel-480x320.json;HEADUNIT_NO_BLUETOOTH=1;QT_QUICK_CONTROLS_STYLE=Basic")
endfunction()

headunit_test(config LABELS unit)
headunit_test(config_fuzz LABELS unit fuzz)
headunit_test(storage LABELS unit)
headunit_test(phone LABELS unit)
headunit_test(application LABELS unit)
headunit_test(presenter LABELS unit)
headunit_test(runtime LABELS unit)
headunit_test(bluetooth_list LABELS unit)
headunit_test(bluetooth_agent LABELS integration DBUS)
headunit_test(ui LABELS ui LIBS Qt6::Gui Qt6::Qml Qt6::Quick SOURCES UiFixture.hpp)
headunit_test(ui_stress LABELS ui stress LIBS Qt6::Gui Qt6::Qml Qt6::Quick SOURCES UiFixture.hpp)
set(HEADUNIT_SOAK_TIMEOUT 600 CACHE STRING "Seconds allowed for the fuzz and stress tests (raise for soak runs)")
set_tests_properties(ui_stress config_fuzz PROPERTIES TIMEOUT ${HEADUNIT_SOAK_TIMEOUT})

# The real executable started the way labwc starts it, in good and bad environments.
add_test(NAME smoke COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/smoke.sh $<TARGET_FILE:headunit>)
set_tests_properties(smoke PROPERTIES LABELS "smoke" TIMEOUT 180)
