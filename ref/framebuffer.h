#pragma once

#include "objects.hh"

// 1. forward declarations
class Texture;

class DisplayFrameImpl;
typedef class DisplayFrameImpl* display_frame_t;

class DisplayDeviceImpl;
typedef class DisplayDeviceImpl* display_device_t;

// 2. api

#include <memory>

class Framebuffer
{
        ENFORCE_ID_OBJECT(Framebuffer);

public:
        Framebuffer();
        virtual ~Framebuffer();

        /**
         * Obtains the last framebuffer's frame as a texture.
         */
        Texture const& asTexture () const;
        void on() const;
        void off() const;

private:
        class Impl;
        std::unique_ptr<Impl> impl;
};

class WithFramebufferScope
{
        ENFORCE_ID_OBJECT(WithFramebufferScope);
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
        Framebuffer& framebuffer;
};
