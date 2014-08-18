#pragma once

#include "objects.hh"

#include <GL/glew.h>

#include <memory>

class Texture
{
        ENFORCE_UNIQUE_REF_OBJECT(Texture);
public:
        Texture() : ref(0)
        {
                glGenTextures(1, &ref);
        }


        Texture(Texture&& other) : ref(0)
        {
                *this = std::move(other);
        }

        Texture& operator=(Texture&& other)
        {
                if (ref != other.ref) {
                        Texture old(ref);
                }

                ref = other.ref;
                other.ref = 0;
                return *this;
        }

        ~Texture()
        {
                glDeleteTextures(1, &ref);
                ref = 0;
        }

        GLuint ref;

private:
        Texture(GLuint ref) : ref (ref) {}
};

