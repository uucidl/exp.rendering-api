#include "material.h"

class MaterialImpl
{};

void material_delete(MaterialImpl* mat)
{}

std::unique_ptr<MaterialImpl, void (*)(MaterialImpl*)> material_make()
{
        return { nullptr, material_delete };
}

void material_commit_with(MaterialImpl* material, int flags, float argb[4])
{}

void material_on(MaterialImpl* material)
{}

void material_off(MaterialImpl* material)
{}
