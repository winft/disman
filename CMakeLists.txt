cmake_minimum_required(VERSION 3.0)

project(Disman)
set(PROJECT_VERSION "0.519.80")
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
include(ECMGenerateHeaders)
include(ECMQtDeclareLoggingCategory)
include(ECMAddQch)
include(FeatureSummary)
include(CheckCXXCompilerFlag)
include(CMakePackageConfigHelpers)
include(GenerateExportHeader)
include(KDEClangFormat)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED Core DBus Gui Test X11Extras)

# Wayland backend plugin KWayland
find_package(KF5Wayland ${KF5_MIN_VERSION} CONFIG REQUIRED)
add_feature_info("KF5Wayland" KF5Wayland_FOUND "Required for building Disman's KWayland backend")

# Wayland backend plugin KWinFT and wlroots
find_package(Wrapland ${PROJECT_VERSION} CONFIG REQUIRED)
add_feature_info("Wrapland" Wrapland_FOUND "Required for building Disman's KWayland backend")

# xrandr backend

find_package(XCB COMPONENTS XCB RANDR)
set_package_properties(XCB PROPERTIES
                       TYPE OPTIONAL
                       PURPOSE "Required for building XRandR backends"
)

option(BUILD_QCH "Build API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)" OFF)
add_feature_info(QCH ${BUILD_QCH} "API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)")

ecm_setup_version(${PROJECT_VERSION} VARIABLE_PREFIX Disman
                        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/disman_version.h"
                        PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/DismanConfigVersion.cmake"
                        SOVERSION ${PROJECT_VERSION_MAJOR}
)

check_cxx_compiler_flag(-fvisibility=hidden _HAVE_VISIBILITY)
if (_HAVE_VISIBILITY AND NOT WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")

  check_cxx_compiler_flag(-fvisibility-inlines-hidden _HAVE_VISIBILITY_INLINES)
  if (_HAVE_VISIBILITY_INLINES)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden")
  endif (_HAVE_VISIBILITY_INLINES)
endif (_HAVE_VISIBILITY AND NOT WIN32)

add_subdirectory(src)
add_subdirectory(backends)
if(BUILD_TESTING)
   add_subdirectory(autotests)
   add_subdirectory(tests)
endif()

set(CMAKECONFIG_INSTALL_DIR "${CMAKECONFIG_INSTALL_PREFIX}/Disman")

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

if (BUILD_QCH)
    ecm_install_qch_export(
        TARGETS Disman_QCH
        FILE DismanQchTargets.cmake
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel
    )
    set(PACKAGE_INCLUDE_QCHTARGETS "include(\"\${CMAKE_CURRENT_LIST_DIR}/DismanQchTargets.cmake\")")
endif()

configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/DismanConfig.cmake.in"
                              "${CMAKE_CURRENT_BINARY_DIR}/DismanConfig.cmake"
                              INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/DismanConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/DismanConfigVersion.cmake"
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT Devel
)

install(EXPORT
  DismanTargets
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  FILE DismanTargets.cmake
  COMPONENT Devel
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/disman_version.h"
  DESTINATION "${INCLUDE_INSTALL_DIR}"
  COMPONENT Devel
)
install(FILES Disman.categories  DESTINATION  ${KDE_INSTALL_LOGGINGCATEGORIESDIR})


feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
