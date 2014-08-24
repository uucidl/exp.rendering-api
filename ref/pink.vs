#version 150

in vec4 position;
in vec2 texcoord;
out vec2 factor;

void main()
{
        gl_Position = position;
        factor = texcoord;
}
