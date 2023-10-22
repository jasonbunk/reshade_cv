#!/usr/bin/env python3
# Copyright (C) 2023 Jason Bunk
import os
import argparse
import math
from glob import glob
from functools import partial
from tqdm.contrib.concurrent import process_map
import numpy as np
from convert_game_snapshot_jsons_to_nerf_transformsjson import convert_image_with_cam_json_to_nerf_transform
from colmap_database import COLMAPDatabase


# source: https://github.com/colmap/colmap/blob/ff8842e7d9e985bd0dd87169f61d5aaeb309ab32/scripts/python/read_write_model.py#L545
def rotmat2qvec(R):
  Rxx, Ryx, Rzx, Rxy, Ryy, Rzy, Rxz, Ryz, Rzz = R.flat
  K = np.array([
      [Rxx - Ryy - Rzz, 0, 0, 0],
      [Ryx + Rxy, Ryy - Rxx - Rzz, 0, 0],
      [Rzx + Rxz, Rzy + Ryz, Rzz - Rxx - Ryy, 0],
      [Ryz - Rzy, Rzx - Rxz, Rxy - Ryx, Rxx + Ryy + Rzz]]) / 3.0
  eigvals, eigvecs = np.linalg.eigh(K)
  qvec = eigvecs[[3, 0, 1, 2], np.argmax(eigvals)]
  if qvec[0] < 0:
    qvec *= -1
  return qvec


if __name__ == '__main__':
  parser = argparse.ArgumentParser()
  parser.add_argument("-fovv", "--fov_degrees_vertical", type=float, default=-999.0, help="provide if game snapshots didnt contain")
  parser.add_argument("-fovh", "--fov_degrees_horizontal", type=float, default=-999.0, help="provide if game snapshots didnt contain")
  parser.add_argument("-o", "--output_folder", type=str, required=True)
  parser.add_argument("-i", "--imagefiles", nargs="+", help="next to each image should be a _meta.json")
  args = parser.parse_args()
  imagefiles = []
  for fpth in args.imagefiles:
    if not os.path.exists(fpth) and ('*' in fpth or '?' in fpth):
      imagefiles.extend(glob(fpth))
    else:
      imagefiles.append(fpth)
  imagefiles = sorted(list(imagefiles))
  for fpth in imagefiles:
    assert os.path.isfile(fpth), fpth

  os.makedirs(args.output_folder, exist_ok=True)

  intrins_to_cam_id = {}
  cam_id_to_intrins = {}
  cam_id_to_image_extr = {}
  centroid = []

  for (camtransf, camheader) in process_map(partial(convert_image_with_cam_json_to_nerf_transform,
    fovv=args.fov_degrees_vertical, fovh=args.fov_degrees_horizontal), imagefiles):

    centroid.append(camtransf['transform_matrix'][:3,3])

    fov_v_deg = math.degrees(camheader["camera_angle_y"])
    pinhole_param = (int(round(fov_v_deg*1000.)), camheader["w"], camheader["h"])

    if pinhole_param not in intrins_to_cam_id:
      new_cam_id = len(intrins_to_cam_id) + 1
      intrins_to_cam_id[pinhole_param] = new_cam_id
      cam_id_to_intrins[new_cam_id] = camheader
      cam_id_to_image_extr[new_cam_id] = []

    cam_id_to_image_extr[intrins_to_cam_id[pinhole_param]].append(camtransf)

  num_images = len(centroid)

  # Centroid of nerf: simply the centroid of the camera locations.
  # Nvidia's colmap2nerf does a fancier calculation
  centroid = np.stack(centroid)
  scale = np.std(centroid, axis=0).max()
  centroid = np.mean(centroid, axis=0)
  if abs(scale - 1.0) < 0.3:
    scale = 1.0
  print("centroid "+str(centroid)+", scale "+str(scale))

  for cid in list(cam_id_to_image_extr.keys()):
    for ii in range(len(cam_id_to_image_extr[cid])):
      cam_id_to_image_extr[cid][ii]['transform_matrix'][:3,3] -= centroid

  db = COLMAPDatabase.connect(os.path.join(args.output_folder, "database.db"))
  db.create_tables()

  # Most games use SIMPLE_PINHOLE camera model
  simple_pinhole_model_id = 0 # see colmap source "src/base/camera_models.h"

  with open(os.path.join(args.output_folder, "cameras.txt"), "w") as outfile:
    outfile.write("# Camera list with one line of data per camera:\n")
    outfile.write("#   CAMERA_ID, MODEL, WIDTH, HEIGHT, PARAMS[]\n")
    outfile.write(f"# Number of cameras: {len(cam_id_to_intrins)}\n")
    for cam_id, intrins in cam_id_to_intrins.items():
      outfile.write(f"{cam_id} SIMPLE_PINHOLE {intrins['w']} {intrins['h']} {intrins['fl_x']} {intrins['cx']} {intrins['cy']}\n")
      db.add_camera(simple_pinhole_model_id, intrins['w'], intrins['h'],
        (intrins['fl_x'], intrins['cx'], intrins['cy']),
        prior_focal_length=True,
        camera_id=cam_id)

  # coordinate system conversion from nerf transforms.json to colmap
  w2cnerf2colmap = np.float64([
     [0, 1, 0, 0],
     [1, 0, 0, 0],
     [0, 0,-1, 0],
     [0, 0, 0, 1]
  ])
  flip_mat = np.float64([
    [1, 0, 0, 0],
    [0, -1, 0, 0],
    [0, 0, -1, 0],
    [0, 0, 0, 1]
  ])

  with open(os.path.join(args.output_folder, "images.txt"), "w") as outfile:
    outfile.write("# Image list with two lines of data per image:\n")
    outfile.write("#   IMAGE_ID, QW, QX, QY, QZ, TX, TY, TZ, CAMERA_ID, NAME\n")
    outfile.write("#   POINTS2D[] as (X, Y, POINT3D_ID)\n")
    outfile.write(f"# Number of images: {num_images}, mean observations per image: 0\n")
    img_id = 1
    for cam_id, list_of_extr in cam_id_to_image_extr.items():
      for extr in list_of_extr:
        w2c = np.matmul(np.linalg.inv(np.matmul(extr["transform_matrix"], flip_mat)), w2cnerf2colmap)
        qvec = rotmat2qvec(w2c[:3,:3])
        tvec = w2c[:3,3]
        outfile.write(f"{img_id} {qvec[0]} {qvec[1]} {qvec[2]} {qvec[3]} {tvec[0]} {tvec[1]} {tvec[2]} {cam_id} {extr['file_path']}\n")
        outfile.write("\n") # no "point observations" will be given
        db.add_image(extr['file_path'], cam_id, prior_q=qvec, prior_t=tvec, image_id=img_id)
        img_id += 1
  assert (img_id-1) == num_images, f"{img_id-1} vs {num_images}"

  db.commit()
  db.close()

  with open(os.path.join(args.output_folder, "points3D.txt"), "w") as outfile:
    pass
