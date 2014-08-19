#version 150

in vec4 position;
in vec2 texcoord;
out vec2 f_texcoords;

void main()
{
        gl_Position = position;
        f_texcoords = texcoord;
}
