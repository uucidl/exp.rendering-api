#include "estd.hpp"
#include "hstd.hpp"
#include "inlineshaders.hpp"
#include "razors-common.hpp"
#include "razorsV2.hpp"

#include "../gl3companion/glresource_types.hpp"
#include "../gl3companion/glinlines.hpp"
#include "../gl3texture/quad.hpp"
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

struct Rect {
        float x;
        float y;
        float width;
        float height;
};

struct QuadDefinerParams {
        Rect coords;
        Rect uvcoords;
};

static
size_t quadDefiner(BufferResource const& elementBuffer,
                   BufferResource const arrays[],
                   void const* data)
{
        auto params = static_cast<QuadDefinerParams const*> (data);
        auto& coords = params->coords;
        auto& uvcoords = params->uvcoords;

        size_t indicesCount = define2dQuadIndices(elementBuffer);
        define2dQuadBuffer(arrays[0],
                           coords.x, coords.y,
                           coords.width, coords.height);
        define2dQuadBuffer(arrays[1],
                           uvcoords.x, uvcoords.y,
                           uvcoords.width, uvcoords.height);

        return indicesCount;
}

void draw(RazorsV2& self, double ms)
{
        auto resolution = viewport();

        auto transparentWhite = [](float alpha) -> std::vector<float> {
                return { alpha*1.0f, alpha*1.0f, alpha*1.0f, alpha };
        };

#if 0
        auto grey = [](float value) -> std::vector<float> {
                // of course this is not real grey, need to go through
                // the standard gamma + color correction instead
                return { value, value, value, 1.0f };
        };
#endif

        auto scaleTransform = [](float scale) -> std::vector<float> {
                return std::vector<float> {
                        scale, 0.01f, 0.0f, 0.0f,
                        -0.01f, scale, 0.0f, 0.0f,
                        0.0f, 0.0f, scale, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f,
                };
        };

        auto quad = [](Rect coords, Rect uvcoords) -> GeometryDef {
                auto geometry = GeometryDef {};

                geometry.arrayCount = 2;
                geometry.data.resize(sizeof(QuadDefinerParams));

                auto params = new (&geometry.data.front()) QuadDefinerParams;
                *params = {
                        .coords = coords,
                        .uvcoords = uvcoords,
                };

                geometry.definer = quadDefiner;

                return geometry;
        };

        auto fullscreenQuad = [quad]() -> GeometryDef {
                return quad(Rect { -1.f, -1.f, 2.0f, 2.0f }, Rect {0.f, 0.f, 1.f, 1.f });
        };

        auto identityMatrix = []() -> std::vector<float> {
                return {
                        1.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 1.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 1.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f
                };
        };


        auto projector = [=](TextureDef const& texture,
        float scale, std::pair<GLint, GLint> viewport) -> RenderObjectDef {
                return RenderObjectDef {
                        .inputs = ProgramInputs {
                                {
                                        { "position", 2 },
                                        { "texcoord", 2 },
                                },
                                {
                                        {
                                                "tex", texture
                                        }
                                },
                                {
                                        ProgramInputs::FloatInput { .name = "g_color", .values = transparentWhite(0.9998f)},
                                        ProgramInputs::FloatInput { .name = "transform", .values = scaleTransform(scale), .last_row = 3 },
                                        ProgramInputs::FloatInput { .name = "iResolution", .values = { (float) viewport.first, (float) viewport.second, 0.0 } },
                                },
                                {},

                        },
                        .geometry = fullscreenQuad()
                };
        };

        auto seed = RenderObjectDef {
                .inputs = ProgramInputs {
                        {
                                { "position", 2 }, { "texcoord", 2 }
                        },
                        {
                                {
                                        .name = "tex", TextureDef {
                                                {},
                                                256,
                                                256,
                                                12,
                                                (TextureDefFn) seed_texture,
                                        }
                                },
                        },
                        {
                                { .name = "depth", .values = { (float)(0.5 * (1.0 + sin(TAU * ms / 3000.0))) } } ,
                                { .name = "transform", .values = identityMatrix(), .last_row = 3 },
                                { .name = "g_color", .values = transparentWhite(0.06f) },
                        },
                        {},
                },
                .geometry = fullscreenQuad(),
        };


        auto vertexShader = [](std::string content) -> VertexShaderDef {
                return VertexShaderDef { .source = content };
        };

        auto fragmentShader = [](std::string content) -> FragmentShaderDef {
                return FragmentShaderDef { .source = content };
        };

        static auto output = makeFrameSeries();
        static auto previousFrame = TextureDef {
                .width = 512,
                .height = 512,
        };
        static auto resultFrame = TextureDef {
                .width = 1024,
                .height = 1024,
        };

        auto const clearFragments = FragmentOperationsDef {
                FragmentOperationsDef::CLEAR, {}
        };
        auto const blendFragments = FragmentOperationsDef {
                FragmentOperationsDef::BLEND_PREMULTIPLIED_ALPHA, {}
        };
        auto const clearAndBlendFragments = FragmentOperationsDef {
                FragmentOperationsDef::CLEAR | FragmentOperationsDef::BLEND_PREMULTIPLIED_ALPHA, {}
        };

        auto const seedProgram = ProgramDef {
                .vertexShader = vertexShader(seedVS),
                .fragmentShader = fragmentShader(seedFS),
        };

        auto const projectorProgram = ProgramDef {
                .vertexShader = vertexShader(defaultVS),
                .fragmentShader = fragmentShader(projectorFS),
        };

        beginFrame(*output);

        previousFrame = drawManyIntoTexture
                        (*output, previousFrame, clearAndBlendFragments,
        projectorProgram, {
                projector(resultFrame, 0.990f + 0.010f * sin(TAU * ms / 5000.0), resolution),
        });
        previousFrame = drawManyIntoTexture
        (*output, previousFrame, blendFragments, seedProgram, {
                seed,
        });

        resultFrame = drawManyIntoTexture
                      (*output, resultFrame, clearFragments, projectorProgram,
        { projector(previousFrame, 1.004f, resolution) });

        drawMany(*output, clearFragments, projectorProgram,
        { projector(resultFrame, 1.0f, resolution) });
}
