#include "material.h"

#include <GL/glew.h>

class MaterialImpl
{
public:
        int flags;
};

void material_delete(MaterialImpl* mat)
{
        delete mat;
}

std::unique_ptr<MaterialImpl, void (*)(MaterialImpl*)> material_make()
{
        return { new MaterialImpl(), material_delete };
}

void material_commit_with(MaterialImpl* material, int flags, float argb[4])
{
        material->flags = flags;
}

void material_on(MaterialImpl* material)
{
        auto flags = material->flags;

        if (flags & MF_BLEND) {
                glEnable(GL_BLEND);
                glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDisable (GL_DEPTH_TEST);
                glDepthMask (GL_FALSE);
        } else {
                glDisable(GL_BLEND);
        }

        if (flags & MF_NO_DEPTH_TEST) {
                glDisable (GL_DEPTH_TEST);
                glDepthMask (GL_FALSE);
        } else {
                glEnable (GL_DEPTH_TEST);
                glDepthMask (GL_TRUE);
                glDepthFunc(GL_LESS);
        }
}

void material_off(MaterialImpl* material)
{
        if (material->flags & MF_BLEND) {
                glDisable(GL_BLEND);
        }
        glDisable (GL_DEPTH_TEST);
        glDepthMask (GL_TRUE);
}
