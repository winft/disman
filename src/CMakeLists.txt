include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${QT_INCLUDES})

add_subdirectory(backendlauncher)
add_subdirectory(doctor)
set(disman_SRCS
    abstractbackend.cpp
    backendmanager.cpp
    config.cpp
    configoperation.cpp
    getconfigoperation.cpp
    setconfigoperation.cpp
    configmonitor.cpp
    configserializer.cpp
    generator.cpp
    screen.cpp
    output.cpp
    edid.cpp
    mode.cpp
    log.cpp
)

qt5_add_dbus_interface(disman_SRCS ${CMAKE_SOURCE_DIR}/interfaces/org.kwinft.disman.backend.xml backendinterface)

ecm_qt_declare_logging_category(disman_SRCS
                                HEADER disman_debug.h
                                IDENTIFIER DISMAN
                                CATEGORY_NAME disman
)

ecm_qt_declare_logging_category(disman_SRCS
                                HEADER disman_debug_edid.h
                                IDENTIFIER DISMAN_EDID
                                CATEGORY_NAME disman.edid
)


add_library(Disman SHARED ${disman_SRCS})
generate_export_header(Disman)

target_link_libraries(Disman
    PUBLIC
    Qt5::Core
    Qt5::Gui
    PRIVATE
    Qt5::DBus
    Qt5::X11Extras
)

set_target_properties(Disman PROPERTIES
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${Disman_SOVERSION}"
    EXPORT_NAME Disman
)

target_include_directories(Disman PUBLIC "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/Disman>")

install(TARGETS Disman EXPORT DismanTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

ecm_generate_headers(Disman_HEADERS
    HEADER_NAMES
        Log
        Mode
        Output
        EDID
        Screen
        Config
        ConfigMonitor
        ConfigOperation
        Generator
        GetConfigOperation
        SetConfigOperation
        Types
    PREFIX Disman
    REQUIRED_HEADERS Disman_REQ_HEADERS
)

install(FILES ${Disman_HEADERS}
        DESTINATION ${INCLUDE_INSTALL_DIR}/Disman/Disman
        COMPONENT Devel)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/disman_export.h
              backendmanager_p.h # needed for unit-tests in Disman
              ${Disman_REQ_HEADERS}
        DESTINATION ${INCLUDE_INSTALL_DIR}/Disman/disman)

if(NOT WIN32)
   configure_file(${CMAKE_CURRENT_SOURCE_DIR}/disman.pc.in ${CMAKE_CURRENT_BINARY_DIR}/disman.pc @ONLY)
   install(FILES ${CMAKE_CURRENT_BINARY_DIR}/disman.pc DESTINATION ${KDE_INSTALL_LIBDIR}/pkgconfig)
endif(NOT WIN32)

include(ECMGeneratePriFile)
ecm_generate_pri_file(BASE_NAME Disman LIB_NAME Disman DEPS "core" FILENAME_VAR PRI_FILENAME)
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})

if(BUILD_QCH)
    ecm_add_qch(
        Disman_QCH
        NAME Disman
        VERSION ${PROJECT_VERSION}
        ORG_DOMAIN org.kwinft
        SOURCES # using only public headers, to cover only public API
            ${Disman_REQ_HEADERS}
        # MD_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md"
        LINK_QCHS
            Qt5Core_QCH
            Qt5Gui_QCH
        BLANK_MACROS
            DISMAN_EXPORT
            DISMAN_DEPRECATED
            DISMAN_DEPRECATED_EXPORT
        TAGFILE_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        QCH_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        COMPONENT Devel
    )
endif()
