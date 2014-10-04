#include "quad.hpp"

#include "../gl3companion/glresource_types.hpp"

#include "stb_perlin.h"

size_t define2dQuadIndices(BufferResource const& buffer)
{
        size_t count = 0;
        withElementBuffer(buffer,
        [&count]() {
                GLuint data[] = {
                        0, 1, 2, 2, 3, 0,
                };
                count = sizeof data / sizeof data[0];

                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof data, data,
                             GL_STREAM_DRAW);
        });

        return count;
}

GLvoid* define2dQuadBuffer(BufferResource const& buffer, float xmin,
                           float ymin, float width, float height)
{
        withArrayBuffer(buffer,
        [=]() {
                float data[] = {
                        xmin, ymin,
                        xmin, ymin + height,
                        xmin + width, ymin + height,
                        xmin + width, ymin,
                };

                glBufferData(GL_ARRAY_BUFFER, sizeof data, data, GL_STREAM_DRAW);
        });

        return 0;
}

extern void perlinNoisePixelFiller (uint32_t* data, int width, int height)
{
        for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                        uint8_t values[4];
                        for (size_t i = 0; i < sizeof values / sizeof values[0]; i++) {
                                float val = 0.5 + stb_perlin_noise3(13.0 * x / width, 17.0 * y / height, 0.0);
                                if (val > 1.0) {
                                        val = 1.0;
                                } else if (val < 0.0) {
                                        val = 0.0;
                                }

                                values[i] = (int) (255 * val) & 0xff;
                        }

                        data[x + y*width] = (values[0] << 24)
                                            | (values[1] << 16)
                                            | (values[2] << 8)
                                            | values[3];
                }
        }
}
