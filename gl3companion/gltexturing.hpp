#pragma once

#include <cstdint>
#include <functional>

/**
 * call while a texture is bound to define a non mipmapped floating point
 * 2d texture.
 */
void defineNonMipmappedFloatTexture(int const width, int const height);


/**
 * call while a texture bound to define a non mipmapped 2d texture
 *
 * pixels are layed out in rows of width pixels from 0 to height
 */
void defineNonMipmappedARGB32Texture(
        int const width, int const height,
        std::function<void(uint32_t* pixels, int width, int height)> pixelFiller);

/**
 * call while a texture is bound to define a non mipmapped 3d texture
 *
 * pixels are layed out in layers from 0 to depth
 */
void defineNonMipmappedARGB32Texture3d(int const width,
                                       int const height,
                                       int const depth,
                                       std::function<void(uint32_t* pixels, int width, int height, int depth)>
                                       pixelFiller);
