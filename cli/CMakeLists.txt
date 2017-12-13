project(konexCLI)

file(GLOB_RECURSE SRC_FILES RELATIVE ${PROJECT_SOURCE_DIR} *.cpp)

add_executable(konex ${SRC_FILES})

target_link_libraries(konex ${Readline_LIBRARY} konexLib)

set_target_properties(konex PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})