cmake_minimum_required(VERSION 3.16)
project(acp_robot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_executable(acp_robot WIN32
  src/main.cpp
  src/acp.cpp
  src/commands.cpp
  src/config.cpp
  src/filedialog.cpp
  src/window.cpp
)

target_compile_options(acp_robot PRIVATE /utf-8)

target_link_libraries(acp_robot PRIVATE user32 gdi32 shell32)
