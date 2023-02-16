cmake_minimum_required(VERSION 3.16)

project(Disman)
set(PROJECT_VERSION "0.527.80")
set(PROJECT_VERSION_MAJOR "0")

set(QT_MIN_VERSION "5.15.0")
set(KF5_MIN_VERSION "5.91")
set(KDE_COMPILERSETTINGS_LEVEL "5.84")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

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
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED
  COMPONENTS
    Core
    DBus
    Gui
    Test
  OPTIONAL_COMPONENTS
    X11Extras
)
set_package_properties(Qt5X11Extras PROPERTIES
  PURPOSE "Required for X11/RandR backend"
)

find_package(KF5 ${KF5_MIN_VERSION} COMPONENTS
  CoreAddons
  Wayland
)
set_package_properties(KF5CoreAddons PROPERTIES
  TYPE REQUIRED
  PURPOSE "For loading Wayland backend plugins"
)
set_package_properties(KF5Wayland PROPERTIES
  TYPE OPTIONAL
  PURPOSE "Required for KWin/KWayland backend"
)

find_package(Wrapland ${PROJECT_VERSION} CONFIG)
set_package_properties(Wrapland PROPERTIES
  TYPE OPTIONAL
  PURPOSE "Required for KWinFT and wlroots backends"
)

find_package(XCB COMPONENTS XCB RANDR)
set_package_properties(XCB PROPERTIES
  TYPE OPTIONAL
  PURPOSE "Required for X11/RandR backend"
)

add_subdirectory(lib)
add_subdirectory(service)
add_subdirectory(ctl)
add_subdirectory(backends)
if(BUILD_TESTING)
  add_subdirectory(autotests)
  add_subdirectory(tests)
endif()

add_custom_target(disman DEPENDS
  disman::lib
  disman::wllib
  dismanctl
  disman-launcher
  disman-wlplugin
)

install(FILES disman.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
