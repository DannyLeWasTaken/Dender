//
// Created by Danny on 2023-01-15.
//

#ifndef DENDER_TEXTURE_HPP
#define DENDER_TEXTURE_HPP

#include <vuk/Image.hpp>

#include <string>
#include <optional>

struct Texture {
    vuk::Texture vukTexture;
    std::string Type;
    std::string Path;
    int Id;
};

#endif //DENDER_TEXTURE_HPP
