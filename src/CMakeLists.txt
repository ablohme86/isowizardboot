qt_add_executable(isowizard
    main.cpp
    backend.cpp backend.h
    settings.cpp settings.h
    diskoperations.cpp diskoperations.h
)
set_target_properties(isowizard PROPERTIES OUTPUT_NAME IsoWizardBoot)
qt_add_resources(isowizard "translations" PREFIX "/i18n" BASE "i18n" FILES i18n/no-NB.i18n)

set(app_icons)
foreach(size IN ITEMS 16 24 32 48 64 128 256 512)
    set(icon "${PROJECT_SOURCE_DIR}/assets/icons/isowizardboot-${size}.png")
    list(APPEND app_icons "${icon}")
    install(FILES "${icon}"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}x${size}/apps"
        RENAME isowizardboot.png)
endforeach()
qt_add_resources(isowizard "app_icons" PREFIX "/icons"
    BASE "${PROJECT_SOURCE_DIR}/assets/icons" FILES ${app_icons})

qt_add_qml_module(isowizard
    URI isowizard
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

# Use the QtQuick module from Qt 6.
target_link_libraries(isowizard PRIVATE Qt6::Quick Qt6::QuickControls2)

# Install the executable
install(TARGETS isowizard DESTINATION "${CMAKE_INSTALL_BINDIR}")

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(diskoperations_test tests/diskoperations_test.cpp diskoperations.cpp)
    target_include_directories(diskoperations_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(diskoperations_test PRIVATE Qt6::Core Qt6::Test)
    target_link_options(diskoperations_test PRIVATE "LINKER:--wrap=fsync")
    add_test(NAME diskoperations COMMAND diskoperations_test)
    qt_add_executable(ui_test tests/ui_test.cpp backend.cpp backend.h diskoperations.cpp settings.cpp settings.h)
    qt_add_resources(ui_test "test_translations" PREFIX "/i18n" BASE "i18n" FILES i18n/no-NB.i18n)
    target_include_directories(ui_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_definitions(ui_test PRIVATE MAIN_QML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/qml/Main.qml")
    target_link_libraries(ui_test PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Test)
    add_test(NAME ui COMMAND ui_test)
    set_tests_properties(ui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software")
endif()
