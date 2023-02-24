# 3D datasets from games - Get NeRFs, RGBD, & point clouds
Plugin for [reshade](https://github.com/crosire/reshade) which you can drop into any game and save RGBD (color+depth) screenshots!

For supported games, the camera transformation matrix can be extracted, which can be [directly converted](python_threedee/convert_game_snapshot_jsons_to_nerf_transformsjson.py) to "transforms.json" for NeRF.
Also for some games, the depth buffer can be converted from uint32 to a physical distance, which helps produce realistic RGBD images or point clouds.

This 3D data can be used to generate ground truth for 3D DNNs, for example SLAM datasets.

### TODOs
* **Documentation, tutorial**
* Camera data from more games
* **Point cloud tool**: using calibrated depth map, extrinsic & intrinsic matrix from supported games.
* Tool to move camera/player to systematically scan large areas to create a synthetic dataset for "Block-NeRF"/"Mega-NeRF"

### Supported games

For most games not in this list (games supported by ReShade), the depth buffer should be capturable, which could be used to build monocular depth datasets.

|Game|Camera matrix? (NeRF ready?)|Point cloud? (Calibrated depth?)|
|----|--------------|-------------------|
|Crysis|Yes|Yes|
|Cyberpunk 2077|Yes|Yes|
|Horizon Zero Dawn|Yes|Yes|
|Resident Evil (RE2R, RE3R)|Yes|Yes|
|Witcher 3|Yes|Yes|

### Sample data
Sample data computed using the camera matrix extracted from the game.
For NeRFs this data can be directly saved as transforms.json (no COLMAP since we have the ground truth!):

#### Cyberpunk 2077
![cyberpunk_nerf](https://user-images.githubusercontent.com/6532938/212845074-bf320377-5b56-429f-b47a-eb2238f684a2.gif)
[![Cyberpunk Point Cloud](https://img.youtube.com/vi/E-JLTHHH_pk/0.jpg)](https://youtu.be/E-JLTHHH_pk)

#### Horizon Zero Dawn
[![HZD NeRF](https://img.youtube.com/vi/7MRoxrtSn0k/0.jpg)](https://youtu.be/7MRoxrtSn0k)

# Build notes

I use Visual Studio 2022, with dependencies installed via vcpkg target ``x64-windows``.

[ReShade 5.6.0](https://github.com/crosire/reshade) is downloaded and placed alongside this repo folder in `..\reshade-5.6.0`

vcpkg dependencies:

* [`eigen3`](https://eigen.tuxfamily.org/) License: MPL2.
* [`nlohmann-json`](https://github.com/nlohmann/json) License: MIT.

## Installation

After using vcpkg to install the dependencies, build this with Visual Studio.

For some games, I use a mod plugin to read the camera matrix. Check the `mod_scripts` folder in this repo for a script for your game; it should have some comments to help lead you to installing for that game.

Visual studio produces a *.addon for ReShade; that should be copied to your game folder (alongside ReShade.log and the game executable).

## Other software included in this repo

Other useful software is already included here and in the Visual Studio build. No need to download them.

* [`fpzip`](https://github.com/LLNL/fpzip) for lossless compression of the floating point depth buffer. (You can load the images in python using [this helpful pypi package](https://github.com/seung-lab/fpzip)). License: BSD 3-Clause.
* [`concurrentqueue`](https://github.com/cameron314/concurrentqueue) for a thread safe parallel queue. License: Simplified BSD.
* [`cnpy`](https://github.com/rogersce/cnpy.git) slightly modified to remove *.npz support to remove dependency on zlib. License: MIT.