project(konexLib)

file(GLOB_RECURSE SRC_FILES RELATIVE ${PROJECT_SOURCE_DIR} *.cpp)

add_library(konexLib ${SRC_FILES})
