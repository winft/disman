if(KF5Wayland_FOUND)
  add_subdirectory(kwayland)
endif()

if(Wrapland_FOUND)
  add_subdirectory(kwinft)
  add_subdirectory(wlroots)
endif()
