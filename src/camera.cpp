//
// Created by Danny on 2023-04-12.
//

#include "camera.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

void camera::update() {
    direction.x = cos(glm::radians(yaw) * cos(glm::radians(pitch)));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw) * cos(glm::radians(pitch)));
    front = glm::normalize(direction);
    view = glm::lookAt(position, position + front, up);
}

void camera::update_position(glm::vec3& pos) {
    this->position = position;
}