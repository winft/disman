add_definitions(-DTEST_DATA="${CMAKE_SOURCE_DIR}/autotests/configs/")

add_executable(waylandtestserver
  main.cpp
  server.cpp
  config_reader.cpp
)

target_link_libraries(waylandtestserver
  disman::backend
  WraplandServer
)
