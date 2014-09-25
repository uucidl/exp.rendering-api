#version 150

uniform vec4 translation;
in vec4 position;
in vec2 texcoord;
out vec2 f_texcoords;

void main()
{
        gl_Position = translation + position;
        f_texcoords = texcoord;
}
