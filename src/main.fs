#version 150

uniform sampler2D tex;
uniform vec4 g_color;

smooth in vec2 ftexcoord;
out vec4 color;

void main()
{
        color = g_color * texture(tex, ftexcoord);
}
