#include <GL/glew.h>

#include <cstdint>
#include <functional>
#include <vector>

void defineNonMipmappedARGB32Texture(
        int const width, int const height,
        std::function<void(uint32_t* pixels, int width, int height)> pixelFiller)
{
        // no mipmapping
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (!pixelFiller) {
                glTexImage2D(GL_TEXTURE_2D,
                             0,
                             GL_RGBA,
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

        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     width,
                     height,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_INT_8_8_8_8_REV,
                     &pixels.front());
}

