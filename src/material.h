#pragma once

#include "objects.hh"

class MaterialImpl;

enum MATERIAL_FLAGS {
        MF_BLEND_POWER,
        MF_DEPTH_TEST_POWER,
        MF_TEXTURE_REPEAT_POWER,

        MF_BLEND = 1 << MF_BLEND_POWER,
        MF_NO_DEPTH_TEST = 1 << MF_DEPTH_TEST_POWER,
        MF_TEXTURE_REPEAT = 1 << MF_TEXTURE_REPEAT_POWER,
};

#include <memory>

std::unique_ptr<MaterialImpl, void (*)(MaterialImpl*)> material_make();
void material_commit_with(MaterialImpl* material, int flags, float argb[4]);
void material_on(MaterialImpl* material);
void material_off(MaterialImpl* material);

class Material
{
        ENFORCE_ID_OBJECT(Material);
public:

        Material() : impl(material_make()) {}

        void commitWith(int flags, float argb[4])
        {
                material_commit_with(impl.get(), flags, argb);
                color[0] = argb[0];
                color[1] = argb[1];
                color[2] = argb[2];
                color[3] = argb[3];
        }

        void on() const
        {
                material_on(impl.get());
        }

        void off() const
        {
                material_off(impl.get());
        }

        float color[4];
private:
        std::unique_ptr<MaterialImpl, void (*)(MaterialImpl*)> impl;
};

class WithMaterialOn
{
        ENFORCE_ID_OBJECT(WithMaterialOn);
public:
        explicit WithMaterialOn(Material const& material)
        {
                material.on();
                current = &material;
        }

        ~WithMaterialOn()
        {
                current->off();
        }

private:
        Material const* current;
};
