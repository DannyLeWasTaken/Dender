cmake_minimum_required(VERSION 3.9)
project(Dender
        VERSION 1.0.0
        DESCRIPTION "Danny's Renderer")
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
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp
        GIT_TAG        v5.2.5
)
FetchContent_MakeAvailable(assimp)

set(VUK_USE_DXC OFF CACHE BOOL "" FORCE) # Don't use mid
FetchContent_Declare(
        vuk
        GIT_REPOSITORY https://github.com/martty/vuk
        GIT_TAG        be7cf2cab527df1310bfe39a65d72c524fdd265
)
FetchContent_MakeAvailable(vuk)

# Create executable

#add_executable(Dender ${SRC_FILES})
add_executable(Dender
        src/main.cpp
        src/GlfwHelper.hpp
        src/utils.hpp
        src/App.cpp src/App.hpp)


# Link included libraries
target_link_libraries(${PROJECT_NAME} PRIVATE vuk)
target_link_libraries(${PROJECT_NAME} PRIVATE vk-bootstrap glfw glm assimp)