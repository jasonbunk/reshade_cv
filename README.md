# DNN datasets from games - NeRF, RGBD, point clouds, semantic segmentation
Plugin for [reshade](https://github.com/crosire/reshade) which you can drop into any game and get some training data!

Supported data varies for each game, but a goal is to support lots of games, because DNNs are data hungry!

Example applications include SLAM, monocular depth, or semantic segmentation. Monocular depth has the broadest support across games thanks to Reshade's depth buffer detection.

## Semantic Segmentation Support

Semantic segmentation is tricky, and currently only DirectX 10 and 11 games are supported. Currently, an effort for each game needs to be undertaken to map mesh/shader IDs to semantic categories of interest. Once a game is sufficiently mapped, large amounts of training data could be gathered from that game. A semi-automated tool is needed here to bootstrap off of an existing detector (e.g. a COCO segmentation DNN).

Note: *Instance* segmentation is not really working right now: instance IDs are extracted, but many objects are drawn in parts, so something would be needed to associate e.g. a car's wheels to its body.

Games I've tested that seem to work include The Witcher 3, Crysis, Control, and Dishonored: DOTO.

In some games, some objects may currently be missing (not segmented): for example Fallout 4 and Shadow of the Tomb Raider.

|Game API|Semantic Segmentation?|
|--------|----------------------|
|DirectX 10|Yes for many games|
|DirectX 11|Yes for many games|
|DirectX 12|Not (yet?) supported|
|Vulkan/OpenGL|Not (yet?) supported|

## 3D support (NeRF / point cloud)

For most games not in this list (games supported by ReShade), the depth buffer should be capturable, which could be used to build monocular depth datasets.

|Game|Camera matrix? (NeRF ready?)|Point cloud? (Calibrated depth?)|
|----|--------------|-------------------|
|Crysis|Yes|Yes|
|Cyberpunk 2077|Yes|Yes|
|Horizon Zero Dawn|Yes|Yes|
|Resident Evil (RE2R, RE3R)|Yes|Yes|
|Witcher 3|Yes|Yes|

### TODOs
Useful functionality is currently working, but this is a work in progress.

* Documentation, tutorial
* Camera data from more games
* Point cloud python script: using calibrated depth map, extrinsic & intrinsic matrix from supported games.
* Segmentation tool to help map mesh/shader IDs to a semantic category schema such as COCO.
* Tool to move camera/player to systematically scan large areas to create big datasets.

## Examples

NeRFs below were computed using the camera matrix extracted from the game, directly saved as transforms.json (no COLMAP since we have ground truth!).

### Semantic Segmentation samples

3D semantic segmentation
![witcher3_3D_segmentation](https://user-images.githubusercontent.com/6532938/232350112-9277bc37-c01f-4832-a106-599092a5b1e8.gif)

![DishonoredDOTO_2023-04-16_681772915_RGB_depth_semseg](https://user-images.githubusercontent.com/6532938/232349914-f988089c-eab4-498e-aeb0-41da440967b0.jpg)

### Cyberpunk 2077 NeRF
![cyberpunk_nerf](https://user-images.githubusercontent.com/6532938/212845074-bf320377-5b56-429f-b47a-eb2238f684a2.gif)
[![Cyberpunk Point Cloud](https://img.youtube.com/vi/E-JLTHHH_pk/0.jpg)](https://youtu.be/E-JLTHHH_pk)

### Horizon Zero Dawn NeRF
[![HZD NeRF](https://img.youtube.com/vi/7MRoxrtSn0k/0.jpg)](https://youtu.be/7MRoxrtSn0k)

### Crysis data (cloud / segmentation / NeRF)

Harbor: 383 images, 3 times of day, 1080p: [Google Drive download (2 GB)](https://drive.google.com/file/d/1qeNEMXdliYBKauxdoas8jQUCPR3MsSxv/view?usp=sharing)
(Includes precomputed Instant-NGP NeRF, and everything needed to recreate, including transforms.json and depth supervision)

<a href="https://drive.google.com/file/d/1qeNEMXdliYBKauxdoas8jQUCPR3MsSxv/view?usp=sharing"><img src="https://github.com/jasonbunk/reshade_cv/assets/6532938/c9b5c234-6bab-42a4-9588-3adae85b9fca" width="600"></a>

# Build notes

I use Visual Studio 2022, with dependencies installed via vcpkg target ``x64-windows``.

[ReShade 5.8.0](https://github.com/crosire/reshade) is downloaded and placed alongside this repo folder in `..\reshade-5.8.0`

vcpkg dependencies:

* [`eigen3`](https://eigen.tuxfamily.org/) License: MPL2.
* [`nlohmann-json`](https://github.com/nlohmann/json) License: MIT.
* [`xxhash`](https://github.com/Cyan4973/xxHash) License: BSD-2.

## Installation

Use vcpkg to install the dependencies.

For semantic segmentation, I use part of [`RenderDoc`](https://github.com/baldurk/renderdoc) (License: MIT), which is included in this project. Real time visualization of the segmentation map is provided by the reshade shader ``reshade_shaders/segmentation_visualization.fx``.

For some games, I use a mod plugin to read the camera matrix. Check the `mod_scripts` folder in this repo for a script for supported games; it should have some comments to help lead you to installing for that game.

Visual studio produces a *.addon for ReShade; that should be copied to your game folder (alongside the game executable and ReShade.log).

## Other software included in this repo

Other useful software is already included here and in the Visual Studio build. No need to download them.

* [`fpzip`](https://github.com/LLNL/fpzip) for lossless compression of the floating point depth buffer. (You can load the images in python using [this helpful pypi package](https://github.com/seung-lab/fpzip)). License: BSD 3-Clause.
* [`concurrentqueue`](https://github.com/cameron314/concurrentqueue) for a thread safe parallel queue. License: Simplified BSD.
* [`cnpy`](https://github.com/rogersce/cnpy.git) slightly modified to remove *.npz support to remove dependency on zlib. License: MIT.
