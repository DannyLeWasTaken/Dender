#include <iostream>
#include "App.hpp"

int main() {
    App* application = new App();
    std::cout << "Running 1" << std::endl;
    application->setup();
    //application->LoadSceneFromFile("C:/Users/Danny/Documents/glTF-Sample-Models/2.0/Box/glTF/Box.gltf");
	application->LoadSceneFromFile("C:/Users/Danny/Documents/glTF-Sample-Models/2.0/Suzanne/glTF/Suzanne.gltf");


    // Begin render loop
    application->loop();
    return 0;
}
