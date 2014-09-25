#version 150

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform vec4 g_color;

in vec2 f_texcoords;
out vec4 color;

void main()
{
        color = g_color + texture(tex0, f_texcoords);
}