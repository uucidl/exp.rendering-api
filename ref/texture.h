#pragma once

#include "objects.hh"

class Texture;

class WithTexture2DBoundScope
{
        ENFORCE_ID_OBJECT(WithTexture2DBoundScope);
public:
        WithTexture2DBoundScope(Texture const& texture);
        ~WithTexture2DBoundScope();
};

