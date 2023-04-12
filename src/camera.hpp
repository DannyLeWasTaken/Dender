//
// Created by Danny on 2023-04-12.
//

#ifndef DENDER_CAMERA_HPP
#define DENDER_CAMERA_HPP

#include <glm/matrix.hpp>

class camera {
public:
    void update();
    void update_position(glm::vec3& pos);
private:
    glm::vec3 position{0, 0, 1};
    glm::vec3 front{0};
    glm::vec3 direction{0};
    glm::vec3 up {0, 1, 0};
    glm::mat4 view;

    // Euler angles
    float pitch;
    float yaw = -90.f;
};


#endif //DENDER_CAMERA_HPP
