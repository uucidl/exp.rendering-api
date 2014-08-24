#pragma once

#include <cstdio>
#include <cstdarg>

/**
 * some generic opengl tools.
 */

static int opengl_print_error (char const * const file, int line,
                               GLenum errorCode)
{
        if (GL_NO_ERROR != errorCode) {
                const char* errorString;
                switch (errorCode) {
                case GL_INVALID_ENUM:
                        errorString = "GL_INVALID_ENUM: GLenum argument out of range.";
                        break;
                case GL_INVALID_VALUE:
                        errorString = "GL_INVALID_VALUE: Numeric argument out of range.";
                        break;
                case GL_INVALID_OPERATION:
                        errorString = "GL_INVALID_OPERATION: Operation illegal in current state.";
                        break;
                case GL_STACK_OVERFLOW:
                        errorString = "GL_STACK_OVERFLOW: Function would cause a stack overflow. ";
                        break;
                case GL_STACK_UNDERFLOW:
                        errorString = "GL_STACK_UNDERFLOW: Function would cause a stack underflow. ";
                        break;
                case GL_OUT_OF_MEMORY:
                        errorString = "GL_OUT_OF_MEMORY: Not enough memory left";
                        break;
#if defined(GL_INVALID_FRAMEBUFFER_OPERATION_EXT)
                case GL_INVALID_FRAMEBUFFER_OPERATION_EXT:
                        errorString =
                                "GL_INVALID_FRAMEBUFFER_OPERATION_EXT: FBO EXT: Invalid operation.";
                        break;
#endif
                default:
                        errorString = "GL: Unknown error.";
                        break;
                }
                printf ("ERROR: %s:%d %s\n", file, line, errorString);
                return 1;
        }

        return 0;
}

static
inline void opengl_trace (char const * const file, int line,
                          char const* pattern, ...)
{
        const GLenum errorCode = glGetError();
        if (opengl_print_error (file, line, errorCode)) {
                if (pattern) {
                        va_list pattern_args;
                        va_start (pattern_args, pattern);

                        vprintf (pattern, pattern_args);
                        va_end (pattern_args);
                }
        }
}

#define OGL_TRACE opengl_trace(__FILE__, __LINE__, NULL)
#define OGL_TRACE_WITH(msg,...) opengl_trace(__FILE__, __LINE__, msg "\n", __VA_ARGS__)
#define OGL_PRINT(error) opengl_print_error(__FILE__, __LINE__, error)
