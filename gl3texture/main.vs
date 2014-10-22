#version 150

uniform vec4 translation;
uniform mat4 transform;
in vec4 position;
in vec2 texcoord;
out vec2 f_texcoords;

void main()
{
        mat4 trans = transform;
        if (0.0 == length(trans * vec4(1.0, 1.0, 1.0, 1.0))) {
                trans = mat4(1.0);
        }
        gl_Position = trans * (translation + position);
        f_texcoords = texcoord;
}
