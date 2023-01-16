//
// Created by Danny on 2023-01-15.
//

#ifndef DENDER_VERTEX_HPP
#define DENDER_VERTEX_HPP

#include <glm/vec3.hpp>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec3 TexCoord;
    int Id;
};

#endif //DENDER_VERTEX_HPP
