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

uniform vec3 iResolution;
uniform sampler2D tex;
uniform vec4 g_color;

smooth in vec2 ftexcoord;
out vec4 color;

void main()
{
vec2 aspectRatioCorrection = vec2(1.0f, 1.0f * iResolution.x / iResolution.y);
vec2 center = vec2(0.5f, 0.5f);
float distance = distance(aspectRatioCorrection * ftexcoord, center);
float scale = clamp(0.02 / distance / distance * max(1.0, 2.0 - pow(2.0*(distance - 0.22), 2.0)), 0.0, 1.0);
color = scale * g_color * texture(tex, ftexcoord);
}
)SHADER";
