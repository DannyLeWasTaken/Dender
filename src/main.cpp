#if _WIN32 || _WIN64
#if _WIN64
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

#include <iostream>
#include "App.hpp"

int main() {
#ifdef ENVIRONMENT32
    throw std::runtime_error("Running x86. Please run x64 instead of x86.");
#endif
    App* application = new App();
    std::cout << "Running 1" << std::endl;
    application->LoadSceneFromFile("C:/Users/Danny/Downloads/glTF-Sample-Models-master/glTF-Sample-Models-master/2.0/Sponza/glTF/Sponza.gltf");
    application->setup();
    return 0;
}
