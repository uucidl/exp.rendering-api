#include "razors-common.hpp"

#include "compiler.hpp"

BEGIN_NOWARN_BLOCK

#include "stb_perlin.h"

END_NOWARN_BLOCK

#include <cstdint>
#include <cstddef>

static void perlin_noise(uint32_t* data, int width, int height,
                         float zplane=0.0f)
{
        for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                        uint8_t values[4];
                        float alpha = 0.0;
                        for (size_t i = 0; i < sizeof values / sizeof values[0]; i++) {
                                float val = 0.5f + stb_perlin_noise3((float)(13.0 * x / width),
                                                                     (float)(17.0 * y / height),
                                                                     zplane);
                                if (i == 0) {
                                        alpha = val;
                                } else {
                                        val *= alpha;
                                }

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

void seed_texture(uint32_t* pixels, int width, int height, int depth)
{
        auto plane = 0.0f;
        for (int d = 0; d < depth; d++) {
                perlin_noise(pixels + d*width*height, width, height, plane);
                plane += 0.2f;
        }
}
