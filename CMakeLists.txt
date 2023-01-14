cmake_minimum_required(VERSION 3.7)
project(Dender)
set(CMAKE_CXX_STANDARD 20)

#includes
include(FetchContent)

# Required packages
find_package(Vulkan REQUIRED)

# Add libraries
FetchContent_Declare(
        vk-bootstrap
        GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
        GIT_TAG        01de71738cd206eefe2537882888dc741881ec0b
)
FetchContent_MakeAvailable(vk-bootstrap)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw
        GIT_TAG        3.3.6
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm
        GIT_TAG        0.9.9.8
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
        fastgltf
        GIT_REPOSITORY https://github.com/spnda/fastgltf
        GIT_TAG        4d6664e02ab3c15be151b5ed880b0c5c52e235e4
)
FetchContent_MakeAvailable(fastgltf)

set(VUK_USE_DXC OFF CACHE BOOL "" FORCE) # Don't use mid
FetchContent_Declare(
        vuk
        GIT_REPOSITORY https://github.com/martty/vuk
        GIT_TAG        be7cf2cab527df1310bfe39a65d72c524fdd265
)
FetchContent_MakeAvailable(vuk)

# Create executable
add_executable(Dender src/main.cpp src/App.cpp src/App.hpp src/Pathtracer.cpp src/Pathtracer.hpp src/utils.hpp)

# Link included libraries
target_link_libraries(${PROJECT_NAME} PRIVATE vuk)
target_link_libraries(${PROJECT_NAME} PRIVATE vk-bootstrap glfw glm fastgltf)