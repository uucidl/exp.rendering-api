#pragma once

#include "objects.hh"

#include <GL/glew.h>

class Buffer
{
public:
        Buffer() : ref(0)
        {
                glGenBuffers(1, &ref);
        }

        Buffer(Buffer&& other) : ref(0)
        {
                *this = std::move(other);
        }

        Buffer& operator=(Buffer&& other)
        {
                if (ref != other.ref) {
                        Buffer old(ref);
                }

                ref = other.ref;
                other.ref = 0;
                return *this;
        }

        ~Buffer()
        {
                glDeleteBuffers(1, &ref);
                ref = 0;
        }


        GLuint ref;

private:
        Buffer(GLuint ref) : ref(ref) {}
        Buffer(Buffer const&)=delete;
        Buffer& operator=(Buffer const&)=delete;
};

class VertexArray
{
public:
        VertexArray() : ref(0)
        {
                glGenVertexArrays(1, &ref);
        }

        VertexArray(VertexArray&& other) : ref(0)
        {
                *this = std::move(other);
        }

        VertexArray& operator=(VertexArray&& other)
        {
                if (ref != other.ref) {
                        VertexArray old (ref);
                }

                ref = other.ref;
                other.ref = 0;
                return *this;
        }

        ~VertexArray()
        {
                glDeleteVertexArrays(1, &ref);
        }

        GLuint ref;

private:
        VertexArray(GLuint ref) : ref(ref) {}
        VertexArray(VertexArray const&) = delete;
        VertexArray& operator=(VertexArray const&) = delete;
};

//! only one such scope allowed at a time
class WithVertexArrayScope
{
        ENFORCE_ID_OBJECT(WithVertexArrayScope);
public:
        WithVertexArrayScope(VertexArray const& va)
        {
                glBindVertexArray(va.ref);
        }

        ~WithVertexArrayScope()
        {
                glBindVertexArray(0);
        }
};

//! only one such scope allowed at a time
class WithArrayBufferScope
{
        ENFORCE_ID_OBJECT(WithArrayBufferScope);
public:
        WithArrayBufferScope(Buffer const& buffer)
        {
                glBindBuffer(GL_ARRAY_BUFFER, buffer.ref);
        }

        ~WithArrayBufferScope()
        {
                glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
};

//! only one such scope allowed at a time
class WithElementArrayBufferScope
{
        ENFORCE_ID_OBJECT(WithElementArrayBufferScope);
public:
        WithElementArrayBufferScope(Buffer const& buffer)
        {
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.ref);
        }

        ~WithElementArrayBufferScope()
        {
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }
};
