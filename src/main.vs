#version 150

in vec4 position;
in vec2 texcoord;

uniform mat4x4 transform;

smooth out vec2 ftexcoord;

void main()
{
        gl_Position = transform * position;
        ftexcoord = texcoord;
}
