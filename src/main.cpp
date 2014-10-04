#include "razors.hpp"
#include "razorsV2.hpp"

#include <GL/glew.h>
#include <micros/api.h>

#include <cmath>
#include <utility>

extern void render_next_2chn_48khz_audio(uint64_t time_micros,
                int const sample_count, double left[/*sample_count*/],
                double right[/*sample_count*/])
{
        // silence is soothing
}

static void draw_changing_background(uint64_t time_micros)
{
        auto phase = 6.30 * time_micros / 1e6 / 1.0;
        float const sincos[2] = {
                static_cast<float> (0.49 * sin(phase)),
                static_cast<float> (0.49 * cos(phase))
        };
        float const argb[4] = {
                0.0f, 0.31f + 0.39f * sincos[0], 0.27f + 0.39f * sincos[1], 0.29f
        };
        glClearColor (argb[1], argb[2], argb[3], argb[0]);
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

extern void render_next_gl3(uint64_t time_micros)
{
        draw_changing_background(time_micros);

        static struct Resources {
                RazorsResource razors { makeRazors() };
                RazorsV2Resource razorsV2 { makeRazorsV2() };
        } all;

        draw(*all.razors, time_micros / 1e3);
        draw(*all.razorsV2, time_micros / 1e3);
}

extern int main()
{
        runtime_init();
}
