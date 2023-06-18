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

#include <stdint.h>
#include <stdio.h>
#include <map>
//#include "api/app/renderdoc_app.h"
#include "api/replay/apidefs.h"
//#include "api/replay/capture_options.h"
//#include "api/replay/control_types.h"
#include "api/replay/stringise.h"
#include "common/timing.h"
#include "os/os_specific.h"

class Chunk;
struct RDCThumb;
struct ReplayOptions;
struct SDObject;

// not provided by tinyexr, just do by hand
bool is_exr_file(FILE *f);
void LogReplayOptions(const ReplayOptions &opts);

enum class RDCDriver : uint32_t;

class IRemoteDriver;
class IReplayDriver;

class StreamReader;
class RDCFile;
struct SDFile;
enum class VulkanLayerFlags : uint32_t;

namespace Callstack
{
class StackResolver;
}

struct ICrashHandler
{
  virtual ~ICrashHandler() {}
  virtual void RegisterMemoryRegion(void *mem, size_t size) = 0;
  virtual void UnregisterMemoryRegion(void *mem) = 0;
};

struct DeviceOwnedWindow
{
  DeviceOwnedWindow() : device(NULL), windowHandle(NULL) {}
  DeviceOwnedWindow(void *dev, void *wnd) : device(dev), windowHandle(wnd) {}
  void *device;
  void *windowHandle;

  bool operator==(const DeviceOwnedWindow &o) const
  {
    return device == o.device && windowHandle == o.windowHandle;
  }
  bool operator<(const DeviceOwnedWindow &o) const
  {
    if(device != o.device)
      return device < o.device;
    return windowHandle < o.windowHandle;
  }

  bool wildcardMatch(const DeviceOwnedWindow &o) const
  {
    if(device == NULL || o.device == NULL)
      return windowHandle == NULL || o.windowHandle == NULL || windowHandle == o.windowHandle;

    if(windowHandle == NULL || o.windowHandle == NULL)
      return device == o.device;

    return *this == o;
  }
};

struct IFrameCapturer
{
  virtual RDCDriver GetFrameCaptureDriver() = 0;
  virtual void StartFrameCapture(DeviceOwnedWindow devWnd) = 0;
  virtual bool EndFrameCapture(DeviceOwnedWindow devWnd) = 0;
  virtual bool DiscardFrameCapture(DeviceOwnedWindow devWnd) = 0;
};

struct IDeviceProtocolHandler;

// In most cases you don't need to check these individually, use the utility functions below
// to determine if you're in a capture or replay state. There are utility functions for each
// state as well.
// See the comments on each state to understand their purpose.
enum class CaptureState
{
  // This is the state while the initial load of a capture is happening and the replay is
  // initialising available resources. This is where any heavy one-off analysis can happen like
  // noting down the details of a action, tracking statistics about resource use and action
  // types, and creating resources that will be needed later in ActiveReplaying.
  //
  // After leaving this state, the capture enters ActiveReplaying and remains there until the
  // capture is closed down.
  LoadingReplaying,

  // After loading, this state is used throughout replay. Whether replaying the frame whole or in
  // part this state indicates that replaying is happening for analysis without the heavy-weight
  // loading process.
  ActiveReplaying,

  // This is the state when no processing is happening - either record or replay - apart from
  // serialising the data. Used with a 'virtual' driver to be able to interpret the contents of a
  // frame capture for structured export without needing to have the API initialised.
  //
  // The idea is that the existing serialisation infrastructure for a driver can be used to decode
  // the raw bits and chunks inside a capture without actually having to be able to initialise the
  // API, and the structured data can then be exported to another format.
  StructuredExport,

  // This is the state while injected into a program for capturing, but no frame is actively being
  // captured at present. Immediately after injection this state is active, and only the minimum
  // necessary work happens to prepare for a frame capture at some later point.
  //
  // When a frame capture is triggered, we immediately transition to the ActiveCapturing state
  // below, where we stay until the frame has been successfully captured, then transition back into
  // this state to continue capturing necessary work in the background for further frame captures.
  BackgroundCapturing,

  // This is the state while injected into a program for capturing and a frame capture is actively
  // ongoing. We transition into this state from BackgroundCapturing on frame capture begin, then
  // stay here until the frame capture is complete and transition back.
  //
  // Note: This state is entered into immediately when a capture is triggered, so it doesn't imply
  // anything about where in the frame we are.
  ActiveCapturing,
};

DECLARE_REFLECTION_ENUM(CaptureState);

constexpr inline bool IsReplayMode(CaptureState state)
{
  return state == CaptureState::LoadingReplaying || state == CaptureState::ActiveReplaying;
}

constexpr inline bool IsCaptureMode(CaptureState state)
{
  return state == CaptureState::BackgroundCapturing || state == CaptureState::ActiveCapturing;
}

constexpr inline bool IsLoading(CaptureState state)
{
  return state == CaptureState::LoadingReplaying;
}

constexpr inline bool IsActiveReplaying(CaptureState state)
{
  return state == CaptureState::ActiveReplaying;
}

constexpr inline bool IsBackgroundCapturing(CaptureState state)
{
  return state == CaptureState::BackgroundCapturing;
}

constexpr inline bool IsActiveCapturing(CaptureState state)
{
  return state == CaptureState::ActiveCapturing;
}

constexpr inline bool IsStructuredExporting(CaptureState state)
{
  return state == CaptureState::StructuredExport;
}

enum class SystemChunk : uint32_t
{
  // 0 is reserved as a 'null' chunk that is only for debug
  DriverInit = 1,
  InitialContentsList,
  InitialContents,
  CaptureBegin,
  CaptureScope,
  CaptureEnd,

  FirstDriverChunk = 1000,
};

DECLARE_REFLECTION_ENUM(SystemChunk);

enum class RDCDriver : uint32_t
{
  Unknown = 0,
  D3D11 = 1,
  OpenGL = 2,
  Mantle = 3,
  D3D12 = 4,
  D3D10 = 5,
  D3D9 = 6,
  Image = 7,
  Vulkan = 8,
  OpenGLES = 9,
  D3D8 = 10,
  Metal = 11,
  MaxBuiltin,
  Custom = 100000,
  Custom0 = Custom,
  Custom1,
  Custom2,
  Custom3,
  Custom4,
  Custom5,
  Custom6,
  Custom7,
  Custom8,
  Custom9,
};

DECLARE_REFLECTION_ENUM(RDCDriver);

struct RDCDriverStatus
{
  bool presenting = false;
  bool supported = false;
  rdcstr supportMessage;

  bool operator==(const RDCDriverStatus &o) const
  {
    return presenting == o.presenting && supported == o.supported &&
           supportMessage == o.supportMessage;
  }
  bool operator!=(const RDCDriverStatus &o) const { return !(*this == o); }
};

enum ReplayLogType
{
  eReplay_Full,
  eReplay_WithoutDraw,
  eReplay_OnlyDraw,
};

DECLARE_REFLECTION_ENUM(ReplayLogType);

enum class VendorExtensions
{
  NvAPI = 0,
  First = NvAPI,
  OpenGL_Ext,
  Vulkan_Ext,
  Count,
};

DECLARE_REFLECTION_ENUM(VendorExtensions);
ITERABLE_OPERATORS(VendorExtensions);

struct CaptureData
{
  rdcstr path;
  rdcstr title;
  uint64_t timestamp = 0;
  RDCDriver driver = RDCDriver::Unknown;
  uint32_t frameNumber = 0;
  bool retrieved = false;
};

enum class LoadProgress
{
  DebugManagerInit,
  First = DebugManagerInit,
  FileInitialRead,
  FrameEventsRead,
  Count,
};

DECLARE_REFLECTION_ENUM(LoadProgress);
ITERABLE_OPERATORS(LoadProgress);

inline constexpr float ProgressWeight(LoadProgress section)
{
  // values must sum to 1.0
  return section == LoadProgress::DebugManagerInit
             ? 0.1f
             : section == LoadProgress::FileInitialRead
                   ? 0.75f
                   : section == LoadProgress::FrameEventsRead ? 0.15f : 0.0f;
}

enum class CaptureProgress
{
  PrepareInitialStates,
  First = PrepareInitialStates,
  // In general we can't know how long the frame capture will take to have an explicit progress, but
  // we can hack it by getting closer and closer to 100% without quite reaching it, with some
  // heuristic for how far we expect to get. Some APIs will have no useful way to update progress
  // during frame capture, but for explicit APIs like Vulkan we can update once per submission, and
  // tune it so that it doesn't start crawling approaching 100% until well past the number of
  // submissions we'd expect in a frame.
  // Other APIs will simply skip this progress section entirely, which is fine.
  FrameCapture,
  AddReferencedResources,
  SerialiseInitialStates,
  SerialiseFrameContents,
  FileWriting,
  Count,
};

DECLARE_REFLECTION_ENUM(CaptureProgress);
ITERABLE_OPERATORS(CaptureProgress);

// different APIs spend their capture time in different places. So the weighting is roughly even for
// the potential hot-spots. So D3D11 might zoom past the PrepareInitialStates while Vulkan takes a
// couple of seconds, but then the situation is reversed for AddReferencedResources
inline constexpr float ProgressWeight(CaptureProgress section)
{
  // values must sum to 1.0
  return section == CaptureProgress::PrepareInitialStates
             ? 0.25f
             : section == CaptureProgress::AddReferencedResources
                   ? 0.25f
                   : section == CaptureProgress::FrameCapture
                         ? 0.15f
                         : section == CaptureProgress::SerialiseInitialStates
                               ? 0.25f
                               : section == CaptureProgress::SerialiseFrameContents
                                     ? 0.08f
                                     : section == CaptureProgress::FileWriting ? 0.02f : 0.0f;
}

// utility function to fake progress with x going from 0 to infinity, mapping to 0% to 100% in an
// inverse curve. For x from 0 to maxX the progress is reasonably spaced, past that it will be quite
// crushed.
//
// The equation is y = 1 - (1 / (x * param) + 1)
//
// => maxX will be when the curve reaches 80%
// 0.8 = 1 - (1 / (maxX * param) + 1)
//
// => gather constants on RHS
// 1 / (maxX * param) + 1 = 0.2
//
// => switch denominators
// maxX * param + 1 = 5
//
// => re-arrange for param
// param = 4 / maxX
inline constexpr float FakeProgress(uint32_t x, uint32_t maxX)
{
  return 1.0f - (1.0f / (x * (4.0f / float(maxX)) + 1));
}

typedef RDResult (*RemoteDriverProvider)(RDCFile *rdc, const ReplayOptions &opts,
                                         IRemoteDriver **driver);
typedef RDResult (*ReplayDriverProvider)(RDCFile *rdc, const ReplayOptions &opts,
                                         IReplayDriver **driver);

typedef RDResult (*StructuredProcessor)(RDCFile *rdc, SDFile &structData);

typedef RDResult (*CaptureImporter)(const rdcstr &filename, StreamReader &reader, RDCFile *rdc,
                                    SDFile &structData, RENDERDOC_ProgressCallback progress);
typedef RDResult (*CaptureExporter)(const rdcstr &filename, const RDCFile &rdc,
                                    const SDFile &structData, RENDERDOC_ProgressCallback progress);
typedef IDeviceProtocolHandler *(*ProtocolHandler)();

typedef bool (*VulkanLayerCheck)(VulkanLayerFlags &flags, rdcarray<rdcstr> &myJSONs,
                                 rdcarray<rdcstr> &otherJSONs);
typedef void (*VulkanLayerInstall)(bool systemLevel);

typedef void (*ShutdownFunction)();

// this class mediates everything and owns any 'global' resources such as the crash handler.
//
// It acts as a central hub that registers any driver providers and can be asked to create one
// for a given logfile or type.
class RenderDoc
{
public:
  struct FramePixels
  {
    uint8_t *data = NULL;
    uint32_t len = 0;
    uint32_t width = 0;
    uint32_t pitch = 0;
    uint32_t height = 0;
    uint32_t stride = 0;
    uint32_t bpc = 0;    // bytes per channel
    bool buf1010102 = false;
    bool buf565 = false;
    bool buf5551 = false;
    bool bgra = false;
    bool is_y_flipped = true;
    uint32_t pitch_requirement = 0;
    uint32_t max_width = 0;
    FramePixels() {}
    ~FramePixels() { SAFE_DELETE_ARRAY(data); }
  };
};

struct DriverRegistration
{
};

struct StructuredProcessRegistration
{
};

struct ConversionRegistration
{
};

struct DeviceProtocolRegistration
{
};
