#pragma once

#include "objects.hh"

#include <GL/glew.h>

class MeshImpl;
class DisplayFrameImpl;

#include <memory>

std::unique_ptr<MeshImpl, void (*)(MeshImpl*)> mesh_make();
void mesh_defquad2d (MeshImpl* g, int flags, float x, float y, float w,
                     float h, float umin, float vmin, float umax, float vmax);
void mesh_bind(MeshImpl* self, GLuint positionAttribLoc,
               GLuint texcoordAttribLoc);
void mesh_draw(MeshImpl* self);

class Mesh
{
        ENFORCE_ID_OBJECT(Mesh);
public:
        Mesh() : impl(mesh_make()) {}

        void defQuad2d(int flags, float x, float y, float w,
                       float h, float umin, float vmin, float umax, float vmax)
        {
                mesh_defquad2d(impl.get(), flags, x, y, w, h, umin, vmin, umax, vmax);
        }

        void bind(GLuint positionAttribLoc, GLuint texcoordAttribLoc)
        {
                mesh_bind(impl.get(), positionAttribLoc, texcoordAttribLoc);
        }

        void draw()
        {
                mesh_draw(impl.get());
        }

private:
        std::unique_ptr<MeshImpl, void (*)(MeshImpl*)> impl;
};
