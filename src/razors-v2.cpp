#include "estd.hpp"
#include "razorsV2.hpp"

#include "../gl3texture/renderer.hpp"

#include <cmath>

static const double TAU =
        6.28318530717958647692528676655900576839433879875021;

class RazorsV2
{};

RazorsV2Resource makeRazorsV2()
{
        return estd::make_unique<RazorsV2>();
}

struct FramebufferDef
{
  std::string id;
  int width;
  int height;
};

struct RenderObjectDef
{
  ProgramInputs inputs;
  GeometryDef geometry;
};

void draw(RazorsV2& self, double ms)
{
  struct Viewport
  {
    int width;
    int height;
  };
  
  auto viewport = Viewport { 100, 100 };
  
  auto frame1 = FramebufferDef {
    .id = "frame1",
    .width = viewport.width,
    .height = viewport.height,
  };

  auto frame2 = FramebufferDef {
    .id = "frame2",
    .width = viewport.width,
    .height = viewport.height,
  };

  auto framebufferTexture = [](FramebufferDef& framebuffer){
    auto texture = TextureDef {};

    texture.data.reserve(sizeof framebuffer);
    *(new (&texture.data.front()) FramebufferDef) = framebuffer;
    texture.width = framebuffer.width;
    texture.height = framebuffer.height;
    texture.pixelFiller = NULL;
    
    return texture;
  };

  auto transparentWhite = [](float alpha) -> std::vector<float> {
    return { alpha*1.0f, alpha*1.0f, alpha*1.0f, alpha };
  };

  auto scaleTransform = [](float scale) -> std::vector<float> {
    return {
      scale, 0.01f, 0.0f, 0.0f,
      -0.01f, scale, 0.0f, 0.0f,
      0.0f, 0.0f, scale, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f,
    };
  };
  
  auto projector = RenderObjectDef {
    .inputs = ProgramInputs {
           {
             { "position" },
               { "texcoord" },
                 },
             {
               { .name = "tex",
                   .content = framebufferTexture(frame2),
               }
             },
               {
                 { .name = "iResolution", .values = { viewport.width, viewport.height } },
                 { .name = "g_color", .values = transparentWhite(0.9998f)},
                 { .name = "transform", .values = scaleTransform(0.990f + 0.010f * sin(TAU * ms / 5000.0)) }
                     }
    },
    .geometry = projectorQuad()
  };

  auto seed = RenderObjectDef {
    .inputs = ProgramInputs {
      {
        { "position" }, { "texcoord " }
      },
      {
        { .name = "tex", seedTexture() },
      },
      .geometry = projectorQuad(),
    };

    
  appendTo(frame1,
         ProgramDef {
           .vertexShader = vertexShader(defaultVS),
             .fragmentShader = fragmentShader(projectorFS),
             },
         {
           projector,
             }
           );
    appendTo(frame1,
             ProgramDef {
               .vertexShader = vertexShader(seedVS),
                 .fragmentShader = fragmentShader(seedFS),
                 },
    { seed });
}
