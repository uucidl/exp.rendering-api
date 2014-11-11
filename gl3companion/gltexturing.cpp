#include "gltexturing.hpp"

#include <GL/glew.h>

#include <cstdint>
#include <functional>
#include <vector>

void defineNonMipmappedFloatTexture(
        int const width, int const height)
{
        auto const target = GL_TEXTURE_2D;
        auto pixelFiller = nullptr;

        // no mipmapping
        glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, 0);

        if (!pixelFiller) {
                glTexImage2D(target,
                             0,
                             GL_RGBA16F,
                             width,
                             height,
                             0,
                             GL_RGBA,
                             GL_UNSIGNED_INT_8_8_8_8_REV,
                             NULL);
                return;
        }

}

// pixels are layed out in rows of width pixels from 0 to height
void defineNonMipmappedARGB32Texture(
        int const width, int const height,
        std::function<void(uint32_t* pixels, int width, int height)> pixelFiller)
{
        auto const target = GL_TEXTURE_2D;

        // no mipmapping
        glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, 0);

        if (!pixelFiller) {
                glTexImage2D(target,
                             0,
                             GL_RGBA8,
                             width,
                             height,
                             0,
                             GL_RGBA,
                             GL_UNSIGNED_INT_8_8_8_8_REV,
                             NULL);
                return;
        }

        std::vector<uint32_t> pixels (width * height);

        pixelFiller(&pixels.front(), width, height);

        glTexImage2D(target,
                     0,
                     GL_RGBA8,
                     width,
                     height,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_INT_8_8_8_8_REV,
                     &pixels.front());
}

/**
 * call while a texture is bound to define a non mipmapped 3d texture
 *
 * pixels are layed out in layers from 0 to depth
 */
void defineNonMipmappedARGB32Texture3d(int const width,
                                       int const height,
                                       int const depth,
                                       std::function<void(uint32_t* pixels, int width, int height, int depth)>
                                       pixelFiller)
{
        auto const target = GL_TEXTURE_3D;

        // no mipmapping
        glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, 0);

        if (!pixelFiller) {
                glTexImage3D(GL_TEXTURE_2D,
                             0,
                             GL_RGBA8,
                             width,
                             height,
                             depth,
                             0,
                             GL_RGBA,
                             GL_UNSIGNED_INT_8_8_8_8_REV,
                             NULL);
                return;
        }

        std::vector<uint32_t> pixels (width * height * depth);

        pixelFiller(&pixels.front(), width, height, depth);

        glTexImage3D(target,
                     0,
                     GL_RGBA8,
                     width,
                     height,
                     depth,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_INT_8_8_8_8_REV,
                     &pixels.front());
}
