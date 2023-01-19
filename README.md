# 3D datasets from games - Get NERFs, RGBD, & point clouds
Plugin for [reshade](https://github.com/crosire/reshade) which you can drop into any game and save RGBD (color+depth) screenshots!

For some supported games, the camera transformation matrix can be extracted, which can be [directly converted](python_threedee/convert_game_snapshot_jsons_to_nerf_transformsjson.py) to "transforms.json" for NERF.
Also for some games, the depth buffer can be converted from uint32 to a physical distance, which helps produce realistic RGBD images or point clouds.

This 3D data can be used to generate ground truth for 3D DNNs, for example SLAM datasets.

### TODOs
* Documentation, tutorial
* Camera data from more games
* Point cloud tool: needs calibrated depth map; Crysis currently works, maybe Horizon Zero Dawn. Cyberpunk depth buffer does a piecewise approximation to the logarithm which is tricky to invert with full precision
* Tool to move camera/player to systematically scan large areas to create a synthetic dataset for "Block-NeRF"/"Mega-NeRF"

### Sample data
Sample NERFs computed using the camera matrix extracted from the game & saved as transforms.json (no COLMAP since we have the ground truth!):

#### Cyberpunk 2077
![cyberpunk_nerf](https://user-images.githubusercontent.com/6532938/212845074-bf320377-5b56-429f-b47a-eb2238f684a2.gif)

#### Horizon Zero Dawn
[![HZD NeRF](https://img.youtube.com/vi/7MRoxrtSn0k/0.jpg)](https://youtu.be/7MRoxrtSn0k)
