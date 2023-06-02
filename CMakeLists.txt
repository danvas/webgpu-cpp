cmake_minimum_required(VERSION 3.1...3.26)

project(
	WebGPUStarter
	VERSION 0.1.0
	LANGUAGES CXX C
)

set(SourceDir src)
set(LibsDir libs)

include(utils.cmake)

add_subdirectory(${LibsDir}/glfw)
add_subdirectory(${LibsDir}/webgpu)
add_subdirectory(${LibsDir}/glfw3webgpu)

add_executable(App
    ${SourceDir}/main.cpp
)

target_compile_definitions(App PRIVATE
		RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources/"
)

# Don't forget to add glfw3webgpu here as well
target_link_libraries(App PRIVATE glfw webgpu glfw3webgpu)

set_target_properties(App PROPERTIES CXX_STANDARD 17)
target_treat_all_warnings_as_errors(App)
target_copy_webgpu_binaries(App)