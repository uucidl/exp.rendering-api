#include "framebuffer.h"

class Framebuffer::Impl
{
public:
        Impl() {}
        ~Impl() {}

        TextureImpl* asTexture()
        {
                return nullptr;
        }

        void on()
        {}

        void off()
        {}
};

Framebuffer::Framebuffer() = default;
Framebuffer::~Framebuffer() = default;

TextureImpl* Framebuffer::asTexture() const
{
        return impl->asTexture();
}

void Framebuffer::on() const
{
        impl->on();
}

void Framebuffer::off() const
{
        impl->off();
}
