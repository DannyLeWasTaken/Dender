cmake_minimum_required(VERSION 3.9)
project(Dender
        VERSION 1.0.0
        DESCRIPTION "Danny's Renderer")
set(CMAKE_CXX_STANDARD 20)

# 64-bit check
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message("Please switch to x64 build.")
    return()
else()
    message("64x build")
endif()

#includes
include(FetchContent)

# Required packages
find_package(Vulkan REQUIRED)

# Add libraries

FetchContent_Declare(
        vuk
        GIT_REPOSITORY https://github.com/martty/vuk
        GIT_TAG        0367df81c99c482fc62a7773ecb85d5286d72cb2
)
set(VUK_USE_DXC OFF CACHE BOOL "" FORCE) # Don't use mid
set(VUK_FAIL_FAST ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(vuk)

FetchContent_Declare(
        vk-bootstrap
        GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
        GIT_TAG        01de71738cd206eefe2537882888dc741881ec0b
)
FetchContent_MakeAvailable(vk-bootstrap)

FetchContent_Declare(
        fastgltf
        GIT_REPOSITORY https://github.com/spnda/fastgltf
        GIT_TAG        v0.4.0
)
FetchContent_MakeAvailable(fastgltf)

FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw
        GIT_TAG        3.3.6
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm
        GIT_TAG        0.9.9.8
)
FetchContent_MakeAvailable(glm)

#set(ASSIMP_BUILD_TESTS ON CACHE BOOL "" FORCE)
#FetchContent_Declare(
#        assimp
#        GIT_REPOSITORY https://github.com/assimp/assimp
#        GIT_TAG        v5.2.5
#)
#set(ASSIMP_NO_EXPORT OFF CACHE BOOL "" FORCE)
#set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
#set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
#set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
#set(ASSIMP_BUILD_ZLIB OFF CACHE BOOL "" FORCE)
#set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
#FetchContent_MakeAvailable(assimp)

# Compiler options
if (MSVC)
    #add_compile_options(/W4)
    message("[WARNING]: AVOID USING MSVC.")
else()
    #add_compile_options("-Wall" "-Wextra" "-pedantic")
    #add_compile_options("-m64" "-g")
endif()


# Create executable

#add_executable(Dender ${SRC_FILES})
add_executable(Dender
        src/main.cpp
        src/stb_image.cc
        src/GlfwHelper.hpp
        src/utils.hpp
        src/App.cpp src/App.hpp
        #src/Assets/Scene.cpp src/Assets/Scene.hpp
        src/Assets/TinyGltfLoader.cpp src/Assets/TinyGltfLoader.hpp src/Assets/TinyGltfLoader.hpp src/Structs/Assets.h src/Assets/fast_gltf_loader.hpp src/Assets/fast_gltf_loader.cpp src/Structs/Assets.hpp src/util/manager.hpp src/util/handle.hpp src/util/manager.cpp src/util/result.hpp src/util/id_gen.hpp)



# Link included libraries
target_link_libraries(${PROJECT_NAME} PRIVATE vuk)
target_link_libraries(${PROJECT_NAME} PRIVATE fastgltf)
target_link_libraries(${PROJECT_NAME} PRIVATE vk-bootstrap glfw glm)