#version 150

uniform sampler2D tex0;
uniform sampler2D tex1;
in vec2 f_texcoords;
out vec4 color;

void main()
{
        color = texture(tex0, f_texcoords);
}