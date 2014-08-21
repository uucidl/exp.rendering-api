#include "razors.h"
#include "matrix.hpp"

#include "frame.h"
#include "framebuffer.h"
#include "material.h"
#include "mesh.h"
#include "shader_types.h"
#include "texture.h"

#include <GL/glew.h>

#include "debug.h"

#include <cmath>

static const double TAU =
        6.28318530717958647692528676655900576839433879875021;

static inline void scale1(matrix4& matrix, double f)
{
        matrix4 t;
        matrix4_identity(t);

        t[0] *= static_cast<float>(-f);
        t[5] *= static_cast<float>(f);

        matrix4_mul(matrix, t);
}

static inline void rotx(matrix4& matrix, double f)
{
        vector4 axis;
        vector4_make(axis, 1.0f, 0.0f, 0.0f, 0.0f);
        vector4 quat;
        quaternion_make_rotation(quat, static_cast<float>(360.0f * f), axis);
        matrix4 rotation;
        matrix4_from_quaternion(rotation, quat);
        matrix4_mul(matrix, rotation);
}

static inline void rotz(matrix4& matrix, double f)
{
        vector4 axis;
        vector4_make(axis, 0.0f, 0.0f, 1.0f, 0.0f);
        vector4 quat;
        quaternion_make_rotation(quat, static_cast<float>(360.0f * f), axis);
        matrix4 rotation;
        matrix4_from_quaternion(rotation, quat);
        matrix4_mul(matrix, rotation);
}

static inline void moveh(matrix4& matrix, double f)
{
        matrix4 t;
        matrix4_identity(t);
        t[3*4 + 0] = static_cast<float>(f);
        matrix4_mul(matrix, t);
}

static void movev(matrix4& matrix, double f)
{
        matrix4 t;
        matrix4_identity(t);
        t[3*4 + 1] = static_cast<float>(f);
        matrix4_mul(matrix, t);
}

static void rdq (display_frame_t frame,
                 ShaderProgram const& shader,
                 Framebuffer const& input,
                 float const cut,
                 int const clear_p)
{
        OGL_TRACE;
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
        WithTexture2DBoundScope bindTexture(input.asTexture());
        if (clear_p) {
                glClearColor (0.0f, 0.0f, 0.0f, 0.0f);
                glClear (GL_COLOR_BUFFER_BIT);
        }

        quad.bind(shader);
        quad.draw();
        OGL_TRACE;
}

static void shaderSetTransform(ShaderProgram const& shader, GLint loc,
                               matrix4 transform)
{
        WithShaderProgramScope withShader(shader);
        glUniformMatrix4fv(loc, 1, GL_FALSE, transform);

}

static void shaderSetMaterial(ShaderProgram const& shader, GLint colorLoc,
                              Material const& material)
{
        WithShaderProgramScope withShader(shader);
        glUniform4f(colorLoc,
                    material.color[1], material.color[2], material.color[3], material.color[0]);
}

static void NF(Framebuffer& output, std::function <void ()> block)
{
        WithFramebufferScope outputFrame(output);
        block();
}


void razors(display_frame_t frame,
            double const ms,
            Material const& mat,
            ShaderProgram const& shader,
            Framebuffer* feedbacks[3],
            float const amplitude,
            float const rot,
            float const blackf,
            int const seed_p,
            Material const& seedmat,
            ShaderProgram const& seedshader)
{
        OGL_TRACE;
        double const phase = ms / 1000.0;

        GLint transformLoc = glGetUniformLocation(shader.ref(), "transform");
        GLint colorLoc = glGetUniformLocation(shader.ref(), "g_color");

        float aa = amplitude;

        {
                WithMaterialOn material(mat);
                {
                        NF(*feedbacks[1], [&]() {
                                matrix4 m = {0};
                                matrix4_identity(m);
                                movev(m, 0.001*cos(phase/50.0));
                                scale1(m, 1.
                                       + 0.001*sin(phase*TAU + TAU/6.0)
                                       + 0.01*aa);
                                rotx(m, 1.0 / 96.0 * (1. + 0.1*sin(phase/7.0 * TAU/3.)));
                                rotz(m, 1.0 / 4.0 * sin(phase * TAU / 33.33));

                                shaderSetMaterial(shader, colorLoc, mat);
                                shaderSetTransform(shader, transformLoc, m);
                                rdq (frame, shader, *feedbacks[0], 1.0f, 1);
                        });
                }

                {
                        NF(*feedbacks[2], [&]() {
                                matrix4 m = {0};
                                matrix4_identity(m);
                                moveh(m, 0.005*sin(phase/500.));
                                scale1(m, 1.0*(1.0 + 0.07*cos(phase*TAU)));
                                rotx(m, 1.0 / 6.0 * (1. + 0.005 * rot));
                                shaderSetMaterial(shader, colorLoc, mat);
                                shaderSetTransform(shader, transformLoc, m);
                                rdq (frame, shader, *feedbacks[0], 1.0f, 1);
                        });
                }
        }
        OGL_TRACE;

        {
                NF(*feedbacks[0], [&]() {
                        {
                                WithMaterialOn material(mat);
                                matrix4 m = {0};
                                matrix4_identity(m);
                                shaderSetMaterial(shader, colorLoc, mat);
                                shaderSetTransform(shader, transformLoc, m);
                                rdq (frame, shader, *feedbacks[0], 1.0f, 0);
                        }

                        {
                                float black_argb[4] = { blackf, 0.0f, 0.0f, 0.0f };

                                Material blacken;
                                blacken.commitWith (MF_BLEND, black_argb);

                                Mesh quad;
                                quad.defQuad2d(0, -1.0f, 1.0f, 2.0f, -2.0f, 0.0f, 0.0f, 1.f, 1.f);

                                {
                                        WithMaterialOn material(blacken);
                                        quad.bind(shader);

                                        quad.draw();
                                }
                        }
                });
                {
                        WithMaterialOn material(mat);
                        NF(*feedbacks[0], [&]() {
                                matrix4 m = {0};
                                matrix4_identity(m);
                                rotz(m, 1./4.*(1. + 0.71 * rot)*cos(phase/10.0)*cos(phase/10.0)*
                                     (1. + 0.01 * aa));
                                shaderSetMaterial(shader, colorLoc, mat);
                                shaderSetTransform(shader, transformLoc, m);
                                rdq (frame, shader, *feedbacks[1], 1.0f, 0);
                        });

                        NF(*feedbacks[0], [&]() {
                                matrix4 m = {0};
                                matrix4_identity(m);
                                shaderSetTransform(shader, transformLoc, m);

                                rdq (frame, shader, *feedbacks[2], 1.0f, 0);
                        });
                }
                if (seed_p) {
                        NF(*feedbacks[0], [&]() {
                                matrix4 m = {0};
                                matrix4_identity(m);
                                shaderSetTransform(shader, transformLoc, m);

                                WithMaterialOn material(seedmat);
                                float uv[] = { 1.0f, 1.0f };

                                Mesh quad;
                                quad.defQuad2d(0, -1.0f, 1.0f, 2.0f, -2.0f,
                                               0.0f, 0.0f, uv[0], uv[1]);

                                quad.bind(seedshader);
                                quad.draw();
                        });
                }
        }
        OGL_TRACE;

        {
                WithMaterialOn material(mat);
                matrix4 m = {0};
                matrix4_identity(m);
                shaderSetTransform(shader, transformLoc, m);
                shaderSetMaterial(shader, colorLoc, mat);
                rdq (frame, shader, *feedbacks[0], 1.0f, 1);
        }
        OGL_TRACE;
}
