#pragma once

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
public:

        Material() : impl(material_make()) {}

        void commitWith(int flags, float argb[4])
        {
                material_commit_with(impl.get(), flags, argb);
        }

        void on() const
        {
                material_on(impl.get());
        }

        void off() const
        {
                material_off(impl.get());
        }

private:
        Material(Material&) = delete;
        Material(Material&&) = delete;
        Material& operator= (Material&) = delete;
        Material& operator= (Material&&) = delete;


        std::unique_ptr<MaterialImpl, void (*)(MaterialImpl*)> impl;
};

class WithMaterialOn
{
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
        WithMaterialOn(WithMaterialOn&) = delete;
        WithMaterialOn(WithMaterialOn&&) = delete;
        WithMaterialOn& operator= (WithMaterialOn&) = delete;
        WithMaterialOn& operator= (WithMaterialOn&&) = delete;

        Material const* current;
};
