add_executable(printconfig testplugandplay.cpp testpnp.cpp)
target_compile_features(printconfig PRIVATE cxx_std_17)
target_link_libraries(printconfig Qt5::Gui Disman::Disman)

if (KF5Wayland_FOUND)
  add_subdirectory(kwayland)
endif()
