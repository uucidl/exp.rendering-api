#pragma once

#include "objects.hh"

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
        ENFORCE_ID_OBJECT(Framebuffer);

public:
        Framebuffer();
        virtual ~Framebuffer();

        /**
         * Obtains the last framebuffer's frame as a texture.
         */
        TextureImpl* asTexture () const;
        void on() const;
        void off() const;

private:
        class Impl;
        std::unique_ptr<Impl> impl;
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
