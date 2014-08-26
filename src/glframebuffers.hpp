#pragma once

#include <utility>

class FramebufferResource;
class TextureResource;
class RenderbufferResource;

/**
 * define a framebuffer resource and its resulting texture and
 * depthbuffer for a given resolution.
 *
 * @param resolution width x height of the desired framebuffer
 * @param framebuffer the framebuffer to define
 * @param framebufferResult the texture to define as the result of the framebuffer
 * @param depthbuffer the render buffer used as the depth buffer
 */
void createFramebuffer(FramebufferResource& framebuffer,
                       TextureResource& framebufferResult,
                       RenderbufferResource& depthbuffer,
                       std::pair<int, int> resolution);
