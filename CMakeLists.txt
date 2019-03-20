cmake_minimum_required(VERSION 3.5)
project(face)

set(CMAKE_CXX_STANDARD 11)

#SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

include_directories(/usr/include/libxml2)
#link_directories(${PROJECT_BINARY_DIR}/lib)

set(EXTRA_LIBS)
#set(EXTRA_LIBS ${EXTRA_LIBS} crypto ssl z pthread xml2 dl sqlite3 uuid)
set(EXTRA_LIBS ${EXTRA_LIBS} crypto ssl z pthread xml2 dl sqlite3)
#set(EXTRA_LIBS ${EXTRA_LIBS} opencv_highgui opencv_imgproc opencv_core)
set(EXTRA_LIBS ${EXTRA_LIBS} opencv_imgcodecs opencv_imgproc opencv_core)

#find_package(SDL REQUIRED)
#if (SDL_FOUND)
#    message(STATUS ${SDL_LIBRARY})
#    include_directories(${SDL_INCLUDE_DIR})
#    set(EXTRA_LIBS ${EXTRA_LIBS} SDL)
#endif (SDL_FOUND)

#configure_file(configure.xml configure.xml)
file(COPY configure.xml face.sqlite DESTINATION .)

aux_source_directory(. DIR_SRCS)

add_executable(face ${DIR_SRCS})

target_link_libraries(face ${EXTRA_LIBS})
