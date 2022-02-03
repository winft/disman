cmake_minimum_required(VERSION 3.13)

project(Disman)
set(PROJECT_VERSION "0.524.80")
set(PROJECT_VERSION_MAJOR "0")

set(QT_MIN_VERSION "5.14.0")
set(KF5_MIN_VERSION "5.66.0")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
include(KDEInstallDirs)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
include(ECMSetupVersion)
include(ECMMarkAsTest)
include(ECMQtDeclareLoggingCategory)
include(FeatureSummary)
include(CMakePackageConfigHelpers)
include(GenerateExportHeader)

# That is the common minimum version. Individual targets might require up to C++17 though.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED
  Core
  DBus
  Gui
  Test
)

# Wayland backend plugin KWayland
find_package(KF5Wayland ${KF5_MIN_VERSION} CONFIG)
set_package_properties(KF5Wayland PROPERTIES
  TYPE OPTIONAL
  PURPOSE "Required for building Disman's KWayland backend"
)

# Wayland backend plugin KWinFT and wlroots
find_package(Wrapland ${PROJECT_VERSION} CONFIG)
set_package_properties(Wrapland PROPERTIES
  TYPE OPTIONAL
  PURPOSE "Required for building Disman's Wrapland-powered backends"
)

# RandR backend
find_package(XCB COMPONENTS XCB RANDR)
set_package_properties(XCB PROPERTIES
  TYPE OPTIONAL
  PURPOSE "Required for building RandR backend."
)

ecm_setup_version(${PROJECT_VERSION}
  VARIABLE_PREFIX Disman
  VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/disman_version.h"
  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/DismanConfigVersion.cmake"
  SOVERSION ${PROJECT_VERSION_MAJOR}
)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)

add_subdirectory(lib)
add_subdirectory(service)
add_subdirectory(ctl)
add_subdirectory(backends)
if(BUILD_TESTING)
  add_subdirectory(autotests)
  add_subdirectory(tests)
endif()

set(CMAKECONFIG_INSTALL_DIR "${CMAKECONFIG_INSTALL_PREFIX}/Disman")

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/DismanConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/DismanConfig.cmake" INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/DismanConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/DismanConfigVersion.cmake"
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT Devel
)

install(
  EXPORT DismanTargets
  FILE DismanTargets.cmake
  NAMESPACE Disman::
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT Devel
)

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/disman_version.h"
  DESTINATION "${INCLUDE_INSTALL_DIR}"
  COMPONENT Devel
)
install(FILES Disman.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
