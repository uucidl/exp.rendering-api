#pragma once

// 1. forward declarations
class FramebufferImpl;
typedef class FramebufferImpl* framebuffer_t;

class TextureImpl;
typedef class TextureImpl* texture_t;

class DisplayFrameImpl;
typedef class DisplayFrameImpl* display_frame_t;

class DisplayDeviceImpl;
typedef class DisplayDeviceImpl* display_device_t;

// 2. api

#include <memory>

std::unique_ptr<FramebufferImpl, void (*)(FramebufferImpl*)>
framebuffer_make();

/**
 * Obtains the last framebuffer's frame as a texture.
 * @relatesalso framebuffer_t
 */
TextureImpl* framebuffer_as_texture (FramebufferImpl* self);

void framebuffer_activate(FramebufferImpl* self);
void framebuffer_deactivate(FramebufferImpl* self);

class Framebuffer
{
public:
        Framebuffer() : impl(framebuffer_make()) {}

        TextureImpl* asTexture () const
        {
                return framebuffer_as_texture(impl.get());
        }

        void on() const
        {
                return framebuffer_activate(impl.get());
        }

        void off() const
        {
                return framebuffer_deactivate(impl.get());
        }

private:
        Framebuffer(Framebuffer&) = delete;
        Framebuffer(Framebuffer&&) = delete;
        Framebuffer& operator=(Framebuffer&) = delete;
        Framebuffer& operator=(Framebuffer&&) = delete;

        std::unique_ptr<FramebufferImpl, void (*)(FramebufferImpl*)> impl;
};

class WithFramebufferScope
{
public:
        WithFramebufferScope(Framebuffer& fb) : framebuffer(fb)
        {
                framebuffer.on();
        }

        ~WithFramebufferScope()
        {
                framebuffer.off();
        }
private:
        WithFramebufferScope(WithFramebufferScope&) = delete;
        WithFramebufferScope(WithFramebufferScope&&) = delete;
        WithFramebufferScope& operator=(WithFramebufferScope&) = delete;
        WithFramebufferScope& operator=(WithFramebufferScope&&) = delete;

        Framebuffer& framebuffer;
};
