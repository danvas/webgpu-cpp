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


# Link libraries
add_subdirectory(${LibsDir}/glfw)
add_subdirectory(${LibsDir}/webgpu)
add_subdirectory(${LibsDir}/glfw3webgpu)
target_link_libraries(App PRIVATE glfw webgpu glfw3webgpu)

# The application's binary must find wgpu.dll or libwgpu.so at runtime,
# so we automatically copy it (it's called WGPU_RUNTIME_LIB in general)
# next to the binary.
target_copy_webgpu_binaries(App)