#include <iostream>
#include "App.hpp"

int main() {
    App* application = new App();
    std::cout << "Running 1" << std::endl;
    application->LoadSceneFromFile("C:/Users/Danny/Downloads/glTF-Sample-Models-master/glTF-Sample-Models-master/2.0/Sponza/glTF/Sponza.gltf");
    std::cout << "Running 2" << std::endl;
    application->setup();
    return 0;
}
