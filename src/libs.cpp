#include "compiler.hpp"

// implementations

#include "../gl3companion/glframebuffers.cpp"
#include "../gl3companion/glresources.cpp"
#include "../gl3companion/glshaders.cpp"
#include "../gl3companion/gltexturing.cpp"
#include "../gl3texture/renderer.cpp"

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

BEGIN_NOWARN_BLOCK

#  define STB_PERLIN_IMPLEMENTATION
#  include "stb_perlin.h"
#  undef STB_PERLIN_IMPLEMENTATION

END_NOWARN_BLOCK
