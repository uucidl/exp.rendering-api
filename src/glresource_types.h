#include <GL/glew.h>

/**
 * Enforce identity semantics for this type name.
 *
 * an object with identity cannot be copied or moved, and must
 * instead be referenced to.
 */
#define ENFORCE_ID_OBJECT(Typename)                     \
        private:                                        \
        Typename(Typename&) = delete;                   \
        Typename(Typename&&) = delete;                  \
        Typename& operator=(Typename&) = delete;        \
        Typename& operator=(Typename&&) = delete;

class TextureResource
{
        ENFORCE_ID_OBJECT(TextureResource);

public:
        TextureResource() : id(0)
        {
                glGenTextures(1, &id);
        }

        ~TextureResource()
        {
                glDeleteTextures(1, &id);
        }

        GLuint id;
};

class FramebufferResource
{
        ENFORCE_ID_OBJECT(FramebufferResource);

public:
        FramebufferResource() : id(0)
        {
                glGenFramebuffers(1, &id);
        }

        ~FramebufferResource()
        {
                glDeleteFramebuffers(1, &id);
        }

        GLuint id;
};

class RenderbufferResource
{
        ENFORCE_ID_OBJECT(RenderbufferResource);

public:
        RenderbufferResource() : id(0)
        {
                glGenRenderbuffers(1, &id);
        }

        ~RenderbufferResource()
        {
                glDeleteRenderbuffers(1, &id);
        }

        GLuint id;
};
