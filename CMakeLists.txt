cmake_minimum_required(VERSION 3.0...3.26)

project(
    WebGPUStarter # project name and name of the visual studio solution (if using it)
    VERSION 0.1.0
    LANGUAGES CXX C # C/C++ project
)

set(RootDir src)
set(MainPath ${RootDir}/main.cpp)
set(LibsDir libs)

message("Building from source: ${MainPath}")
add_executable(App ${MainPath})

set_target_properties(App PROPERTIES
    CXX_STANDARD 17
    COMPILE_WARNING_AS_ERROR ON
)

if (MSVC)
    target_compile_options(App PRIVATE /W4)
else()
    target_compile_options(App PRIVATE -Wall -Wextra -pedantic)
endif()


# Add GLFW library
set(GlfwLibPath ${LibsDir}/glfw)
add_subdirectory(${GlfwLibPath})
target_link_libraries(App PRIVATE glfw)