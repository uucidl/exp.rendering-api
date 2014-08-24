#pragma once

// 1. forward declararations
class DisplayFrameImpl;
typedef class DisplayFrameImpl* display_frame_t;

class Framebuffer;
class Material;
class ShaderProgram;

// 2. API

void razors(display_frame_t frame,
            double ms,
            Material const& mat,
            ShaderProgram const& shader,
            Framebuffer* feedback[3],
            float amplitude,
            float rot,
            float blackf,
            int seed_p,
            Material const& seedmat,
            ShaderProgram const& seedshader);
