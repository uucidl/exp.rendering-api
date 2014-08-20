#version 150

in vec2 factor;
out vec4 color;

void main()
{
        color = vec4(1.0, 0.5 + factor.x, 1.0, 1.0);
}
