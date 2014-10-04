#include "compiler.hpp"

// implementations

#include "../gl3companion/glresources.cpp"
#include "../gl3companion/glframebuffers.cpp"
#include "../gl3companion/glshaders.cpp"
#include "../gl3companion/gltexturing.cpp"


BEGIN_NOWARN_BLOCK

#  define STB_PERLIN_IMPLEMENTATION
#  include "stb_perlin.h"
#  undef STB_PERLIN_IMPLEMENTATION

END_NOWARN_BLOCK
