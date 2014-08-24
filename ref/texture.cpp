#include "texture.h"
#include "texture_types.h"

#include <GL/glew.h>

/// bind texture

WithTexture2DBoundScope::WithTexture2DBoundScope(Texture const& texture)
{
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_2D, texture.ref);
}

WithTexture2DBoundScope::~WithTexture2DBoundScope()
{
        glBindTexture(GL_TEXTURE_2D, 0);
}
