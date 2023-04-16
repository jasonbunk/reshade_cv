// Copyright (C) 2023 Jason Bunk
#pragma once
#include <stdint.h>
#include <string>

struct custom_shader_layout_registers {
    int perdrawbuf_tex_regL = -9;
    int perdrawbuf_tex_regH = -9;
    int rendertarget_index = -9;

    inline std::string to_string() const {
        return std::string("pdbuf_rL ") + std::to_string(perdrawbuf_tex_regL)
            + std::string(", rH ") + std::to_string(perdrawbuf_tex_regH)
            + std::string(", rti ") + std::to_string(rendertarget_index);
    }
};

#ifdef RENDERDOC_FOR_SHADERS
#include "api/replay/rdcarray.h" // renderdoc header
#else
#include <vector>
typedef std::vector<uint8_t> bytebuf;
#endif
