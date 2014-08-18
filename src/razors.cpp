#include "razors.h"

#include "frame.h"
#include "framebuffer.h"
#include "material.h"
#include "mesh.h"
#include "texture.h"

#include <GL/glew.h>

#include <cmath>

static const double TAU =
        6.28318530717958647692528676655900576839433879875021;

/**
 * Clamp a floating point number between two boundaries.
 */
static inline float clamp_f (float const f, float const min_b,
                             float const max_b)
{
        return f < min_b ? min_b : (f > max_b ? max_b : f);
}

static inline void scale1(float f)
{
        glScalef(-f, f, 1.0f);
}

static inline void rotx(float f)
{
        glRotatef(360.0f * f, 1.0f, 0.0f, 0.0f);
}

static inline void rotz(float f)
{
        glRotatef(360.0f * f, 0.0f, 0.0f, 1.0f);
}

static inline void moveh(float f)
{
        glTranslatef(f, 0.0f, 0.f);
}

static void movev(float f)
{
        glTranslatef(0.0f, f, 0.f);
}

#define NF(block) do {						\
                glPushAttrib(GL_SCISSOR_BIT);			\
                glDisable(GL_SCISSOR_TEST);			\
                glMatrixMode(GL_PROJECTION);			\
                glPushMatrix();					\
                glLoadIdentity();				\
                glOrtho(-1.0, 1.0, -1.0, +1.0, +1.0, -1.0);	\
                glMatrixMode(GL_MODELVIEW);			\
                glPushMatrix();					\
                glLoadIdentity();				\
                block;						\
                glMatrixMode(GL_MODELVIEW);			\
                glPopMatrix();					\
                glMatrixMode(GL_PROJECTION);			\
                glPopMatrix();					\
                glPopAttrib();					\
        } while (0)

static void rdq (display_frame_t frame,
                 Framebuffer * const feedback[],
                 float const cut,
                 int const clear_p,
                 int const input)
{
        float uv[2] = {
                1.0f,
                1.0f,
        };

        Mesh quad;

        float border = clamp_f(1.0f - cut, 0.0f, 1.0f);
        float hborder = border / 2.0f;

        quad.defQuad2d(0, -1.0f + border, 1.0f - border, 2.0f - border,
                       -2.0f + border,
                       uv[0]*hborder, uv[1]*hborder, uv[0]*(1.0f - hborder), uv[1]*(1.0f - hborder));

        WithTexture2DBoundScope(feedback[input]->asTexture());
        if (clear_p) {
                glClearColor (0.0f, 0.0f, 0.0f, 0.0f);
                glClear (GL_COLOR_BUFFER_BIT);
        }

        quad.draw();
}

void razors(display_frame_t frame,
            double const ms,
            Material const& mat,
            Framebuffer* feedbacks[3],
            float const amplitude,
            float const rot,
            float const blackf,
            int const seed_p,
            Material const& seedmat)
{
        double const phase = ms / 1000.0f;

        float aa = amplitude;

        {
                WithMaterialOn material(mat);
                {
                        WithFramebufferScope subframe1(*feedbacks[1]);
                        NF(({
                                movev(0.001f*cos(phase/50.0f));
                                scale1(1.0f*(1.f + 0.001f*sin(phase*TAU + TAU/6.0f)
                                + 0.01f*aa));
                                rotx(1.0f / 96.0f * (1.f + 0.1f*sin(phase/7.0 * TAU/3.f)));
                                rotz(1.0f / 4.0f * sin(phase * TAU / 33.33f));
                                rdq (frame, feedbacks, 1.0f, 1, 0);
                        }));
                }

                {
                        WithFramebufferScope subframe2(*feedbacks[2]);
                        NF(({
                                moveh(0.005*sin(phase/500.f));
                                scale1(1.0f*(1.0f + 0.07f*cos(phase*TAU)));
                                rotx(1.0f / 6.0f * (1.f + 0.005f * rot));
                                rdq (frame, feedbacks, 1.0f, 1, 0);
                        }));
                }
        }

        {
                WithFramebufferScope subframe0(*feedbacks[0]);
                NF(({
                        {
                                WithMaterialOn material(mat);
                                rdq (frame, feedbacks, 1.0f, 0, 0);
                        }

                        {
                                float black_argb[4] = { blackf, 0.0f, 0.0f, 0.0f };

                                Material blacken;
                                blacken.commitWith (MF_BLEND, black_argb);

                                Mesh quad;
                                quad.defQuad2d(0, -1.0f, 1.0f, 2.0f, -2.0f, 0.0f, 0.0f, 1.f, 1.f);

                                {
                                        WithMaterialOn material(blacken);
                                        quad.draw();
                                }
                        }
                }));

                {
                        WithMaterialOn material(mat);
                        NF(({
                                rotz(1./4.*(1.f + 0.71f * rot)*cos(phase/10.0f)*cos(phase/10.0f)*(1.f + 0.01f * aa));
                                rdq (frame, feedbacks, 1.0f, 0, 1);
                        }));

                        NF(({
                                rdq (frame, feedbacks, 1.0f, 0, 2);
                        }));
                }
                if (seed_p) {
                        NF(({
                                WithMaterialOn material(seedmat);
                                float uv[] = { 1.0f, 1.0f };

                                Mesh quad;
                                quad.defQuad2d(0, -1.0f, 1.0f, 2.0f, -2.0f,
                                               0.0f, 0.0f, uv[0], uv[1]);

                                quad.draw();
                        }));
                }
        }

        {
                WithMaterialOn material(mat);
                rdq (frame, feedbacks, 1.0f, 1, 0);
        }

}
