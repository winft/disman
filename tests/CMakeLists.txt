add_executable(printconfig testplugandplay.cpp testpnp.cpp)
target_compile_features(printconfig PRIVATE cxx_std_17)
target_link_libraries(printconfig Qt6::Gui disman::lib)

if (Wrapland_FOUND)
  add_subdirectory(wayland)
endif()
