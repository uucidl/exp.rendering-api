#version 150

uniform sampler2D tex0;
uniform sampler2D tex1;
in vec2 f_texcoords;
out vec4 color;

void main()
{
        color = vec4(1.0, 0.7, 0.8, 1.0) + texture(tex0, f_texcoords) + texture(tex1,
                        f_texcoords);
}