static std::string defaultVS = R"SHADER(
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
)SHADER";

static std::string defaultFS = R"SHADER(
#version 150

uniform sampler2D tex;
uniform vec4 g_color;

smooth in vec2 ftexcoord;
out vec4 color;

void main()
{
color = g_color * texture(tex, ftexcoord);
}
)SHADER";

static std::string projectorFS = R"SHADER(
#version 150

uniform sampler2D tex;
uniform vec4 g_color;

smooth in vec2 ftexcoord;
out vec4 color;

void main()
{
vec2 center = vec2(0.5f, 0.5f);
float distance = distance(ftexcoord, center);
float scale = clamp(0.22 / distance / distance, 0.0, 1.0);
color = scale * g_color * texture(tex, ftexcoord);
}
)SHADER";