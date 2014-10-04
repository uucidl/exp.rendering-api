#include "../gl3companion/gldebug.hpp"
#include "../gl3companion/glresource_types.hpp"
#include "../gl3companion/glshaders.hpp"
#include "../gl3companion/gltexturing.hpp"
#include "../src/hstd.hpp"
#include "quad.hpp"

#include <micros/api.h>

#include "stb_image_write.h"

#include <GL/glew.h>

#include <vector>
#include <cmath>

extern void render_textured_quad_v1(uint64_t time_micros);
extern void render_textured_quad_v2(uint64_t time_micros);

extern void render_next_2chn_48khz_audio(uint64_t time_micros,
                int const sample_count, double left[/*sample_count*/],
                double right[/*sample_count*/])
{
        // silence is soothing
}

extern void render_next_gl3(uint64_t time_micros)
{
        render_textured_quad_v1(time_micros);
        render_textured_quad_v2(time_micros);
}

int main()
{
        runtime_init();

        return 0;
}
