cmake_minimum_required(VERSION 3.24)
project(Dender)
set(CMAKE_CXX_STANDARD 17)

#includes
include(FetchContent)

# Required packages

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
        GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)

FETCHCONTENT_DECLARE(
        fastgltf
        GIT_REPOSITORY https://github.com/spnda/fastgltf
        GIT_TAG v0.2.0
)
FETCHCONTENT_MAKEAVAILABLE(fastgltf)

add_executable(Dender main.cpp)

target_link_directories(${PROJECT_NAME} PRIVATE glfw glm fastgltf)