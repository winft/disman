add_executable(dismanctl main.cpp doctor.cpp watcher.cpp)
target_compile_features(dismanctl PRIVATE cxx_std_17)

target_link_libraries(dismanctl
  Qt6::DBus
  disman::lib
)

install(
  TARGETS dismanctl
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT disman
)
