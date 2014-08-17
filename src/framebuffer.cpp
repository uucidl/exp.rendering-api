#include "framebuffer.h"

void framebuffer_destroy(FramebufferImpl* self)
{
}

std::unique_ptr<FramebufferImpl, void (*)(FramebufferImpl*)>
framebuffer_make()
{
        return { nullptr, framebuffer_destroy };
}

/**
 * Obtains the last framebuffer's frame as a texture.
 * @relatesalso framebuffer_t
 */
TextureImpl* framebuffer_as_texture (FramebufferImpl* self)
{
        return nullptr;
}

void framebuffer_activate(FramebufferImpl* self)
{}

void framebuffer_deactivate(FramebufferImpl* self)
{}
