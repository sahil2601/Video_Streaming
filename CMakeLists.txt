cmake_minimum_required(VERSION 3.10)
project(VideoStreamProject)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER_APP REQUIRED gstreamer-app-1.0)

include_directories(${GSTREAMER_INCLUDE_DIRS} ${GSTREAMER_APP_INCLUDE_DIRS})
link_directories(${GSTREAMER_LIBRARY_DIRS} ${GSTREAMER_APP_LIBRARY_DIRS})

add_executable(server server.cpp)
target_link_libraries(server ${GSTREAMER_LIBRARIES} ${GSTREAMER_APP_LIBRARIES})

add_executable(client client.cpp)
target_link_libraries(client ${GSTREAMER_LIBRARIES} ${GSTREAMER_APP_LIBRARIES})
