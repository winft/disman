add_executable(printconfig testplugandplay.cpp testpnp.cpp)
target_link_libraries(printconfig Qt5::Gui Disman)

add_subdirectory(kwayland)
