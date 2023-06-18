/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2019-2023 Baldur Karlsson
 * Copyright (c) 2014 Crytek
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#pragma once

#include "api/replay/shader_types.h"
#include "core/core.h"
#include "maths/vec.h"

enum class RemapTexture : uint32_t
{
  NoRemap,
  RGBA8,
  RGBA16,
  RGBA32
};

DECLARE_REFLECTION_ENUM(RemapTexture);

struct GetTextureDataParams
{
  // this data is going to be saved to disk, so prepare it as needed. E.g. on GL flip Y order to
  // match conventional axis for file formats.
  bool forDiskSave = false;
  // this data is going to be transferred cross-API e.g. in replay proxying, so standardise bit
  // layout of any packed formats where API conventions differ (mostly only RGBA4 or other awkward
  // ones where our resource formats don't enumerate all possible iterations). Saving to disk is
  // also standardised to ensure the data matches any format description we also write to the
  // format.
  bool standardLayout = false;
  CompType typeCast = CompType::Typeless;
  bool resolve = false;
  RemapTexture remap = RemapTexture::NoRemap;
  float blackPoint = 0.0f;
  float whitePoint = 1.0f;
};

DECLARE_REFLECTION_STRUCT(GetTextureDataParams);

CompType BaseRemapType(RemapTexture remap, CompType typeCast);
inline CompType BaseRemapType(const GetTextureDataParams &params)
{
  return BaseRemapType(params.remap, params.typeCast);
}

class RDCFile;

class AMDRGPControl;

struct RenderOutputSubresource
{
  RenderOutputSubresource(uint32_t mip, uint32_t slice, uint32_t numSlices)
      : mip(mip), slice(slice), numSlices(numSlices)
  {
  }

  uint32_t mip, slice, numSlices;
};

void PatchTriangleFanRestartIndexBufer(rdcarray<uint32_t> &patchedIndices, uint32_t restartIndex);

uint64_t CalcMeshOutputSize(uint64_t curSize, uint64_t requiredOutput);

void StandardFillCBufferVariable(ResourceId shader, const ShaderConstantType &desc,
                                 uint32_t dataOffset, const bytebuf &data, ShaderVariable &outvar,
                                 uint32_t matStride);
void StandardFillCBufferVariables(ResourceId shader, const rdcarray<ShaderConstant> &invars,
                                  rdcarray<ShaderVariable> &outvars, const bytebuf &data);
void PreprocessLineDirectives(rdcarray<ShaderSourceFile> &sourceFiles);

extern const Vec4f colorRamp[22];

enum class DiscardType : int
{
  RenderPassLoad,         // discarded on renderpass load
  RenderPassStore,        // discarded after renderpass store
  UndefinedTransition,    // transition from undefined layout
  DiscardCall,            // explicit Discard() type API call
  InvalidateCall,         // explicit Invalidate() type API call
  Count,
};

static constexpr uint32_t DiscardPatternWidth = 64;
static constexpr uint32_t DiscardPatternHeight = 8;
