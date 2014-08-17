#include "mesh.h"

#include "buffer_types.h"
#include "shader_types.h"

class MeshImpl
{
public:
        GLuint programRef;
        Buffer vertices;
        Buffer texcoords;
        Buffer indices;
        size_t indices_n;
        VertexArray array;
};

void mesh_delete(MeshImpl* mesh)
{
        delete mesh;
}

std::unique_ptr<MeshImpl, void (*)(MeshImpl*)> mesh_make()
{
        return { new MeshImpl(), mesh_delete };
}


void mesh_defquad2d (MeshImpl* g, int flags,
                     float x, float y, float w, float h,
                     float umin, float vmin, float umax, float vmax)
{
        {
                WithArrayBufferScope withVertices(g->vertices);
                float vertices[] = {
                        x, y,
                        x, y + h,
                        x + w, y + h,
                        x + w, y,
                };

                glBufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices, GL_STREAM_DRAW);
        }

        {
                WithArrayBufferScope withTexcoords(g->texcoords);
                float texcoords[] = {
                        umin, vmin,
                        umin, vmax,
                        umax, vmax,
                        umax, vmin,
                };

                glBufferData(GL_ARRAY_BUFFER, sizeof texcoords, texcoords, GL_STREAM_DRAW);

        }

        {
                WithElementArrayBufferScope withIndices(g->indices);
                GLuint indices[] = {
                        0, 1, 2, 2, 3, 0,
                };

                g->indices_n = sizeof indices / sizeof indices[0];

                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof indices, indices,
                             GL_STREAM_DRAW);
        }

}

void mesh_bind(MeshImpl* self, ShaderProgram& shader)
{
        self->programRef = shader.ref();

        GLuint position_attrib = glGetAttribLocation(self->programRef, "position");
        GLuint texcoord_attrib = glGetAttribLocation(self->programRef, "texcoord");

        WithVertexArrayScope withVertexArray(self->array);

        glBindBuffer(GL_ARRAY_BUFFER, self->texcoords.ref);
        glVertexAttribPointer(texcoord_attrib, 2, GL_FLOAT, GL_FALSE, 0, 0);

        glBindBuffer(GL_ARRAY_BUFFER, self->vertices.ref);
        glVertexAttribPointer(position_attrib, 2, GL_FLOAT, GL_FALSE, 0, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self->indices.ref);
        glEnableVertexAttribArray(position_attrib);
        glEnableVertexAttribArray(texcoord_attrib);

        shader.validate();
}

void mesh_draw(MeshImpl* self)
{
        glUseProgram(self->programRef);
        WithVertexArrayScope withVertexArray(self->array);
        glDrawElements(GL_TRIANGLES, self->indices_n, GL_UNSIGNED_INT, 0);
        glUseProgram(0);
}
