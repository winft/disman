add_executable(printconfig testplugandplay.cpp testpnp.cpp)
target_link_libraries(printconfig Qt5::Gui Disman::Disman)

if (KF5Wayland_FOUND)
  add_subdirectory(kwayland)
endif()
