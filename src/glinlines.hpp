#pragma once

#include <GL/glew.h>

#include <utility>

/// sometimes we don't really care about double precision
static inline GLfloat glfloat(double x)
{
        return static_cast<float> (x);
}

static inline std::pair<int, int> viewport()
{
        GLint wh[4];
        glGetIntegerv(GL_VIEWPORT, wh);

        return { wh[2], wh[3] };
}

static inline void clear()
{
        glClearColor (0.0, 0.0, 0.0, 0.0);
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
