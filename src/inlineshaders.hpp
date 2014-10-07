#pragma once

#include <string>

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
vec2 aspectRatioCorrection = vec2(1.0f, 1.0f * iResolution.y / iResolution.x);
vec2 center = vec2(0.5f, 0.5f);
float delta = distance(aspectRatioCorrection * (ftexcoord.xy - center), vec2(0,0));
float ring = max(0.0, 1.0 - 1.0/0.004*pow((delta - 0.38), 2.0));
float halo = 1.0 / (1.0 + delta - 0.4) / (1.0 + delta - 0.4);
float scale = clamp(halo + ring, 0.0, 1.0);
color = scale * g_color * texture(tex, ftexcoord);
}
)SHADER";

static std::string seedVS = R"SHADER(
#version 150

in vec4 position;
in vec2 texcoord;

uniform float depth;
uniform mat4x4 transform;

smooth out vec3 ftexcoord;

void main()
{
gl_Position = transform * position;
ftexcoord = vec3(texcoord.x, texcoord.y, depth);
}
)SHADER";

static std::string seedFS = R"SHADER(
#version 150

uniform sampler3D tex;
uniform vec4 g_color;

smooth in vec3 ftexcoord;
out vec4 color;

void main()
{
color = g_color * texture(tex, ftexcoord);
}
)SHADER";
