#pragma once

#include <cstdint>
#include <functional>

/// call while a texture bound to define a non mipmapped 2d texture
void defineNonMipmappedARGB32Texture(
        int const width, int const height,
        std::function<void(uint32_t* pixels, int width, int height)> pixelFiller);
