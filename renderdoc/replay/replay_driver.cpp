/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2019-2023 Baldur Karlsson
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

#include "replay_driver.h"
#include "maths/formatpacking.h"
#include "maths/half_convert.h"
#include "strings/string_utils.h"
#include "common/formatting.h"

template <>
rdcstr DoStringise(const RemapTexture &el)
{
  BEGIN_ENUM_STRINGISE(RemapTexture);
  {
    STRINGISE_ENUM_CLASS(NoRemap)
    STRINGISE_ENUM_CLASS(RGBA8)
    STRINGISE_ENUM_CLASS(RGBA16)
    STRINGISE_ENUM_CLASS(RGBA32)
  }
  END_ENUM_STRINGISE();
}

CompType BaseRemapType(RemapTexture remap, CompType typeCast)
{
  switch(typeCast)
  {
    case CompType::Float:
    case CompType::SNorm:
    case CompType::UNorm: return CompType::Float;
    case CompType::UNormSRGB:
      return remap == RemapTexture::RGBA8 ? CompType::UNormSRGB : CompType::Float;
    case CompType::UInt: return CompType::UInt;
    case CompType::SInt: return CompType::SInt;
    default: return typeCast;
  }
}

void PatchTriangleFanRestartIndexBufer(rdcarray<uint32_t> &patchedIndices, uint32_t restartIndex)
{
  if(patchedIndices.empty())
    return;

  rdcarray<uint32_t> newIndices;

  uint32_t firstIndex = patchedIndices[0];

  size_t i = 1;
  // while we have at least two indices left
  while(i + 1 < patchedIndices.size())
  {
    uint32_t a = patchedIndices[i];
    uint32_t b = patchedIndices[i + 1];

    if(a != restartIndex && b != restartIndex)
    {
      // no restart, add primitive
      newIndices.push_back(firstIndex);
      newIndices.push_back(a);
      newIndices.push_back(b);

      i++;
      continue;
    }
    else if(b == restartIndex)
    {
      // we've already added the last triangle before the restart in the previous iteration, just
      // continue so we hit the a == restartIndex case below
      i++;
    }
    else if(a == restartIndex)
    {
      // new first index is b
      firstIndex = b;
      // skip both the restartIndex value and the first index, and begin at the next real index (if
      // it exists)
      i += 2;

      uint32_t next[2] = {b, b};

      // if this is the last vertex, the triangle will be degenerate
      if(i < patchedIndices.size())
        next[0] = patchedIndices[i];
      if(i + 1 < patchedIndices.size())
        next[1] = patchedIndices[i + 1];

      // output 3 dummy degenerate triangles so vertex ID mapping is easy
      // we rotate the triangles so the important vertex is last in each.
      for(size_t dummy = 0; dummy < 3; dummy++)
      {
        newIndices.push_back(restartIndex);
        newIndices.push_back(restartIndex);
        newIndices.push_back(restartIndex);
      }
    }
  }

  newIndices.swap(patchedIndices);
}

void StandardFillCBufferVariable(ResourceId shader, const ShaderConstantType &desc,
                                 uint32_t dataOffset, const bytebuf &data, ShaderVariable &outvar,
                                 uint32_t matStride)
{
  const VarType type = outvar.type;
  const uint32_t rows = outvar.rows;
  const uint32_t cols = outvar.columns;

  size_t elemByteSize = 4;
  if(type == VarType::Double || type == VarType::ULong || type == VarType::SLong ||
     type == VarType::GPUPointer)
    elemByteSize = 8;
  else if(type == VarType::Half || type == VarType::UShort || type == VarType::SShort)
    elemByteSize = 2;
  else if(type == VarType::UByte || type == VarType::SByte)
    elemByteSize = 1;

  // primary is the 'major' direction
  // so a matrix is a secondaryDim number of primaryDim-sized vectors
  uint32_t primaryDim = cols;
  uint32_t secondaryDim = rows;
  if(rows > 1 && outvar.ColMajor())
  {
    primaryDim = rows;
    secondaryDim = cols;
  }

  if(dataOffset < data.size())
  {
    const byte *srcData = data.data() + dataOffset;
    const size_t avail = data.size() - dataOffset;

    byte *dstData = outvar.value.u8v.data();

    // each secondaryDim element (row or column) is stored in a primaryDim-vector.
    // We copy each vector member individually to account for smaller than uint32 sized types.
    for(uint32_t s = 0; s < secondaryDim; s++)
    {
      for(uint32_t p = 0; p < primaryDim; p++)
      {
        const size_t srcOffset = matStride * s + p * elemByteSize;
        const size_t dstOffset = (primaryDim * s + p) * elemByteSize;

        if(srcOffset + elemByteSize <= avail)
          memcpy(dstData + dstOffset, srcData + srcOffset, elemByteSize);
      }
    }

    // if it's a matrix and not row major, transpose
    if(primaryDim > 1 && secondaryDim > 1 && outvar.ColMajor())
    {
      ShaderVariable tmp = outvar;

      if(elemByteSize == 8)
      {
        for(size_t ri = 0; ri < rows; ri++)
          for(size_t ci = 0; ci < cols; ci++)
            outvar.value.u64v[ri * cols + ci] = tmp.value.u64v[ci * rows + ri];
      }
      else if(elemByteSize == 4)
      {
        for(size_t ri = 0; ri < rows; ri++)
          for(size_t ci = 0; ci < cols; ci++)
            outvar.value.u32v[ri * cols + ci] = tmp.value.u32v[ci * rows + ri];
      }
      else if(elemByteSize == 2)
      {
        for(size_t ri = 0; ri < rows; ri++)
          for(size_t ci = 0; ci < cols; ci++)
            outvar.value.u16v[ri * cols + ci] = tmp.value.u16v[ci * rows + ri];
      }
      else if(elemByteSize == 1)
      {
        for(size_t ri = 0; ri < rows; ri++)
          for(size_t ci = 0; ci < cols; ci++)
            outvar.value.u8v[ri * cols + ci] = tmp.value.u8v[ci * rows + ri];
      }
    }
  }

  if(desc.pointerTypeID != ~0U)
    outvar.SetTypedPointer(outvar.value.u64v[0], shader, desc.pointerTypeID);
}

static void StandardFillCBufferVariables(ResourceId shader, const rdcarray<ShaderConstant> &invars,
                                         rdcarray<ShaderVariable> &outvars, const bytebuf &data,
                                         uint32_t baseOffset)
{
  for(size_t v = 0; v < invars.size(); v++)
  {
    rdcstr basename = invars[v].name;

    uint8_t rows = invars[v].type.rows;
    uint8_t cols = invars[v].type.columns;
    uint32_t elems = RDCMAX(1U, invars[v].type.elements);
    const bool rowMajor = invars[v].type.RowMajor();
    const bool isArray = elems > 1;

    const uint32_t matStride = invars[v].type.matrixByteStride;

    uint32_t dataOffset = baseOffset + invars[v].byteOffset;

    if(!invars[v].type.members.empty() || (rows == 0 && cols == 0))
    {
      ShaderVariable var;
      var.name = basename;
      var.rows = var.columns = 0;
      var.type = VarType::Struct;
      if(rowMajor)
        var.flags |= ShaderVariableFlags::RowMajorMatrix;

      rdcarray<ShaderVariable> varmembers;

      if(isArray)
      {
        var.members.resize(elems);
        for(uint32_t i = 0; i < elems; i++)
        {
          ShaderVariable &vr = var.members[i];
          vr.name = StringFormat::Fmt("%s[%u]", basename.c_str(), i);
          vr.rows = vr.columns = 0;
          vr.type = VarType::Struct;
          if(rowMajor)
            vr.flags |= ShaderVariableFlags::RowMajorMatrix;

          StandardFillCBufferVariables(shader, invars[v].type.members, vr.members, data, dataOffset);

          dataOffset += invars[v].type.arrayByteStride;
        }
      }
      else
      {
        var.type = VarType::Struct;

        StandardFillCBufferVariables(shader, invars[v].type.members, var.members, data, dataOffset);
      }

      outvars.push_back(var);

      continue;
    }

    size_t outIdx = outvars.size();
    outvars.push_back({});

    {
      const VarType type = invars[v].type.baseType;

      outvars[outIdx].name = basename;
      outvars[outIdx].rows = 1;
      outvars[outIdx].type = type;
      outvars[outIdx].columns = cols;
      if(rowMajor)
        outvars[outIdx].flags |= ShaderVariableFlags::RowMajorMatrix;

      ShaderVariable &var = outvars[outIdx];

      if(!isArray)
      {
        outvars[outIdx].rows = rows;

        StandardFillCBufferVariable(shader, invars[v].type, dataOffset, data, outvars[outIdx],
                                    matStride);
      }
      else
      {
        var.name = outvars[outIdx].name;
        var.rows = 0;
        var.columns = 0;

        rdcarray<ShaderVariable> varmembers;
        varmembers.resize(elems);

        rdcstr base = outvars[outIdx].name;

        for(uint32_t e = 0; e < elems; e++)
        {
          varmembers[e].name = StringFormat::Fmt("%s[%u]", base.c_str(), e);
          varmembers[e].rows = rows;
          varmembers[e].type = type;
          varmembers[e].columns = cols;
          if(rowMajor)
            varmembers[e].flags |= ShaderVariableFlags::RowMajorMatrix;

          uint32_t rowDataOffset = dataOffset;

          dataOffset += invars[v].type.arrayByteStride;

          StandardFillCBufferVariable(shader, invars[v].type, rowDataOffset, data, varmembers[e],
                                      matStride);
        }

        var.members = varmembers;
      }
    }
  }
}

void PreprocessLineDirectives(rdcarray<ShaderSourceFile> &sourceFiles)
{
  struct SplitFile
  {
    rdcstr filename;
    rdcarray<rdcstr> lines;
    bool modified = false;
  };

  rdcarray<SplitFile> splitFiles;

  splitFiles.resize(sourceFiles.size());

  for(size_t i = 0; i < sourceFiles.size(); i++)
    splitFiles[i].filename = sourceFiles[i].filename;

  for(size_t i = 0; i < sourceFiles.size(); i++)
  {
    rdcarray<rdcstr> srclines;

    // start off writing to the corresponding output file.
    SplitFile *dstFile = &splitFiles[i];
    bool changedFile = false;

    size_t dstLine = 0;

    // skip this file if it doesn't contain #line
    if(!sourceFiles[i].contents.contains("#line"))
      continue;

    split(sourceFiles[i].contents, srclines, '\n');
    srclines.push_back("");

    // handle #line directives by inserting empty lines or erasing as necessary
    bool seenLine = false;

    for(size_t srcLine = 0; srcLine < srclines.size(); srcLine++)
    {
      if(srclines[srcLine].empty())
      {
        dstLine++;
        continue;
      }

      char *c = &srclines[srcLine][0];
      char *end = c + srclines[srcLine].size();

      while(*c == '\t' || *c == ' ' || *c == '\r')
        c++;

      if(c == end)
      {
        // blank line, just advance line counter
        dstLine++;
        continue;
      }

      if(c + 5 > end || strncmp(c, "#line", 5) != 0)
      {
        // only actually insert the line if we've seen a #line statement. Otherwise we're just
        // doing an identity copy. This can lead to problems e.g. if there are a few statements in
        // a file before the #line we then create a truncated list of lines with only those and
        // then start spitting the #line directives into other files. We still want to have the
        // original file as-is.
        if(seenLine)
        {
          // resize up to account for the current line, if necessary
          dstFile->lines.resize(RDCMAX(dstLine + 1, dstFile->lines.size()));

          // if non-empty, append this line (to allow multiple lines on the same line
          // number to be concatenated). To avoid screwing up line numbers we have to append with
          // a comment and not a newline.
          if(dstFile->lines[dstLine].empty())
            dstFile->lines[dstLine] = srclines[srcLine];
          else
            dstFile->lines[dstLine] += " /* multiple #lines overlapping */ " + srclines[srcLine];

          dstFile->modified = true;
        }

        // advance line counter
        dstLine++;

        continue;
      }

      seenLine = true;

      // we have a #line directive
      c += 5;

      if(c >= end)
      {
        // invalid #line, just advance line counter
        dstLine++;
        continue;
      }

      while(*c == '\t' || *c == ' ')
        c++;

      if(c >= end)
      {
        // invalid #line, just advance line counter
        dstLine++;
        continue;
      }

      // invalid #line, no line number. Skip/ignore and just advance line counter
      if(*c < '0' || *c > '9')
      {
        dstLine++;
        continue;
      }

      size_t newLineNum = 0;
      while(*c >= '0' && *c <= '9')
      {
        newLineNum *= 10;
        newLineNum += int((*c) - '0');
        c++;
      }

      // convert to 0-indexed line number
      if(newLineNum > 0)
        newLineNum--;

      while(*c == '\t' || *c == ' ')
        c++;

      if(*c == '"')
      {
        // filename
        c++;

        char *filename = c;

        // parse out filename
        while(*c != '"' && *c != 0)
        {
          if(*c == '\\')
          {
            // skip escaped characters
            c += 2;
          }
          else
          {
            c++;
          }
        }

        // parsed filename successfully
        if(*c == '"')
        {
          *c = 0;

          rdcstr fname = filename;
          if(fname.empty())
            fname = "shader";

          // find the new destination file
          bool found = false;
          size_t dstFileIdx = 0;

          for(size_t f = 0; f < splitFiles.size(); f++)
          {
            if(splitFiles[f].filename == fname)
            {
              found = true;
              dstFileIdx = f;
              break;
            }
          }

          if(found)
          {
            changedFile = (dstFile != &splitFiles[dstFileIdx]);
            dstFile = &splitFiles[dstFileIdx];
          }
          else
          {
            RDCWARN("Couldn't find filename '%s' in #line directive in debug info", fname.c_str());

            // make a dummy file to write into that won't be used.
            splitFiles.push_back(SplitFile());
            splitFiles.back().filename = fname;
            splitFiles.back().modified = true;

            changedFile = true;
            dstFile = &splitFiles.back();
          }

          // set the next line number, and continue processing
          dstLine = newLineNum;

          continue;
        }
        else
        {
          // invalid #line, ignore
          RDCERR("Couldn't parse #line directive: '%s'", srclines[srcLine].c_str());
          continue;
        }
      }
      else
      {
        // No filename. Set the next line number, and continue processing
        dstLine = newLineNum;
        continue;
      }
    }
  }

  for(size_t i = 0; i < sourceFiles.size(); i++)
  {
    if(sourceFiles[i].contents.empty() || splitFiles[i].modified)
    {
      merge(splitFiles[i].lines, sourceFiles[i].contents, '\n');
    }
  }
}

void StandardFillCBufferVariables(ResourceId shader, const rdcarray<ShaderConstant> &invars,
                                  rdcarray<ShaderVariable> &outvars, const bytebuf &data)
{
  // start with offset 0
  StandardFillCBufferVariables(shader, invars, outvars, data, 0);
}

uint64_t CalcMeshOutputSize(uint64_t curSize, uint64_t requiredOutput)
{
  if(curSize == 0)
    curSize = 32 * 1024 * 1024;

  // resize exponentially up to 256MB to avoid repeated resizes
  while(curSize < requiredOutput && curSize < 0x10000000ULL)
    curSize *= 2;

  // after that, just align the required size up to 16MB and allocate that. Otherwise we can
  // vastly-overallocate at large sizes.
  if(curSize < requiredOutput)
    curSize = AlignUp(requiredOutput, (uint64_t)0x1000000ULL);

  return curSize;
}

// colour ramp from http://www.ncl.ucar.edu/Document/Graphics/ColorTables/GMT_wysiwyg.shtml
const Vec4f colorRamp[22] = {
    Vec4f(0.000000f, 0.000000f, 0.000000f, 0.0f), Vec4f(0.250980f, 0.000000f, 0.250980f, 1.0f),
    Vec4f(0.250980f, 0.000000f, 0.752941f, 1.0f), Vec4f(0.000000f, 0.250980f, 1.000000f, 1.0f),
    Vec4f(0.000000f, 0.501961f, 1.000000f, 1.0f), Vec4f(0.000000f, 0.627451f, 1.000000f, 1.0f),
    Vec4f(0.250980f, 0.752941f, 1.000000f, 1.0f), Vec4f(0.250980f, 0.878431f, 1.000000f, 1.0f),
    Vec4f(0.250980f, 1.000000f, 1.000000f, 1.0f), Vec4f(0.250980f, 1.000000f, 0.752941f, 1.0f),
    Vec4f(0.250980f, 1.000000f, 0.250980f, 1.0f), Vec4f(0.501961f, 1.000000f, 0.250980f, 1.0f),
    Vec4f(0.752941f, 1.000000f, 0.250980f, 1.0f), Vec4f(1.000000f, 1.000000f, 0.250980f, 1.0f),
    Vec4f(1.000000f, 0.878431f, 0.250980f, 1.0f), Vec4f(1.000000f, 0.627451f, 0.250980f, 1.0f),
    Vec4f(1.000000f, 0.376471f, 0.250980f, 1.0f), Vec4f(1.000000f, 0.125490f, 0.250980f, 1.0f),
    Vec4f(1.000000f, 0.376471f, 0.752941f, 1.0f), Vec4f(1.000000f, 0.627451f, 1.000000f, 1.0f),
    Vec4f(1.000000f, 0.878431f, 1.000000f, 1.0f), Vec4f(1.000000f, 1.000000f, 1.000000f, 1.0f),
};
