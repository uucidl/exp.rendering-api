#pragma once

#include <GL/glew.h>

#include <cstdint>
#include <cstddef>

class BufferResource;

extern size_t define2dQuadIndices(BufferResource const& buffer);
extern GLvoid* define2dQuadBuffer(BufferResource const& buffer, float xmin,
                                  float ymin, float width, float height);
extern void perlinNoisePixelFiller (uint32_t* data, int width, int height);
