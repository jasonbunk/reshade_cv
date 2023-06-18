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

#include "settings.h"
#include "api/replay/structured_data.h"
#include "common/formatting.h"
//#include "serialise/streamio.h"
#include "core.h"

//#include "3rdparty/pugixml/pugixml.hpp"

static const rdcliteral debugOnlyString = "DEBUG VARIABLE: Read-only in stable builds."_lit;

static rdcstr valueString(const SDObject *o)
{
  if(o->type.basetype == SDBasic::String)
    return o->data.str;

  if(o->type.basetype == SDBasic::UnsignedInteger)
    return StringFormat::Fmt("%llu", o->data.basic.u);

  if(o->type.basetype == SDBasic::SignedInteger)
    return StringFormat::Fmt("%lld", o->data.basic.i);

  if(o->type.basetype == SDBasic::Float)
    return StringFormat::Fmt("%lf", o->data.basic.d);

  if(o->type.basetype == SDBasic::Boolean)
    return o->data.basic.b ? "True" : "False";

  if(o->type.basetype == SDBasic::Array)
    return StringFormat::Fmt("[%zu]", o->NumChildren());

  return "{}";
}


const bool &ConfigVarRegistration<bool>::value()
{
  // avoid warnings on stupid compilers
  (void)tmp;
  return obj->data.basic.b;
}

const uint64_t &ConfigVarRegistration<uint64_t>::value()
{
  (void)tmp;
  return obj->data.basic.u;
}

const uint32_t &ConfigVarRegistration<uint32_t>::value()
{
  tmp = obj->data.basic.u & 0xFFFFFFFFU;
  return tmp;
}

const rdcstr &ConfigVarRegistration<rdcstr>::value()
{
  tmp = obj->data.str;
  return tmp;
}

template <typename T>
rdcstr DefValString(const T &el)
{
  return ToStr(el);
}

// this one needs a special implementation unfortunately to convert
const rdcarray<rdcstr> &ConfigVarRegistration<rdcarray<rdcstr>>::value()
{
  tmp.resize(obj->NumChildren());
  for(size_t i = 0; i < tmp.size(); i++)
    tmp[i] = obj->GetChild(i)->data.str;

  return tmp;
}

rdcstr DefValString(const rdcarray<rdcstr> &el)
{
  rdcstr ret = "[";
  for(size_t i = 0; i < el.size(); i++)
  {
    if(i != 0)
      ret += ", ";
    ret += el[i];
  }
  ret += "]";
  return ret;
}

inline SDObject *makeSDObject(const rdcinflexiblestr &name, const rdcarray<rdcstr> &vals)
{
  SDObject *ret = new SDObject(name, "array"_lit);
  ret->type.basetype = SDBasic::Array;
  for(const rdcstr &s : vals)
    ret->AddAndOwnChild(makeSDObject("$el"_lit, s));
  return ret;
}

#define CONFIG_SUPPORT_TYPE(T)                                                            \
  ConfigVarRegistration<T>::ConfigVarRegistration(rdcliteral name, const T &defaultValue, \
                                                  bool debugOnly, rdcliteral description) \
  {}

CONFIG_SUPPORT_TYPE(bool)
CONFIG_SUPPORT_TYPE(uint64_t)
CONFIG_SUPPORT_TYPE(uint32_t)
CONFIG_SUPPORT_TYPE(rdcstr)
CONFIG_SUPPORT_TYPE(rdcarray<rdcstr>)



//============================

#include "api/replay/replay_enums.h"
#include "api/replay/renderdoc_tostr.inl"


// not exported, this is needed for calling from the container allocate functions
void RENDERDOC_OutOfMemory(uint64_t sz)
{
    RDCFATAL("Allocation failed for %llu bytes", sz);
}







/////////////////////////////////////////////////////////////
// Basic types

template <>
rdcstr DoStringise(const rdcstr &el)
{
  return el;
}

template <>
rdcstr DoStringise(const rdcinflexiblestr &el)
{
  return el;
}

template <>
rdcstr DoStringise(const rdcliteral &el)
{
  return el;
}

template <>
rdcstr DoStringise(void *const &el)
{
  return StringFormat::Fmt("%#p", el);
}

template <>
rdcstr DoStringise(const int64_t &el)
{
  return StringFormat::Fmt("%lld", el);
}

#if ENABLED(RDOC_SIZET_SEP_TYPE)
template <>
rdcstr DoStringise(const size_t &el)
{
  return StringFormat::Fmt("%llu", (uint64_t)el);
}
#endif

template <>
rdcstr DoStringise(const uint64_t &el)
{
  return StringFormat::Fmt("%llu", el);
}

template <>
rdcstr DoStringise(const uint32_t &el)
{
  return StringFormat::Fmt("%u", el);
}

template <>
rdcstr DoStringise(const char &el)
{
  return StringFormat::Fmt("'%c'", el);
}

template <>
rdcstr DoStringise(const wchar_t &el)
{
  return StringFormat::Fmt("'%lc'", el);
}

template <>
rdcstr DoStringise(const byte &el)
{
  return StringFormat::Fmt("%hhu", el);
}

template <>
rdcstr DoStringise(const int8_t &el)
{
  return StringFormat::Fmt("%hhd", el);
}

template <>
rdcstr DoStringise(const uint16_t &el)
{
  return StringFormat::Fmt("%hu", el);
}

template <>
rdcstr DoStringise(const int32_t &el)
{
  return StringFormat::Fmt("%d", el);
}

template <>
rdcstr DoStringise(const int16_t &el)
{
  return StringFormat::Fmt("%hd", el);
}

template <>
rdcstr DoStringise(const float &el)
{
  return StringFormat::Fmt("%0.4f", el);
}

template <>
rdcstr DoStringise(const double &el)
{
  return StringFormat::Fmt("%0.4lf", el);
}

template <>
rdcstr DoStringise(const bool &el)
{
  if(el)
    return "True";

  return "False";
}
