#!/usr/bin/env python3
# Copyright (C) 2022 Jason Bunk
import os
import argparse
import json
import math
from glob import glob
from PIL import Image
from functools import partial
from tqdm.contrib.concurrent import process_map
import cv2
import numpy as np
from game_camera import (
  focallengthperpixel_fromfovdegrees,
  fovv_and_fovh_degrees_given_either
)


# https://github.com/NVlabs/instant-ngp/blob/a0090e4cad3ce3a5e52ad4239e5f5e2f9c70ebff/scripts/colmap2nerf.py#L144-L148
def sharpness(image: np.ndarray):
  gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
  fm = cv2.Laplacian(gray, cv2.CV_64F).var()
  return fm


def convert_image_with_cam_json_to_nerf_transform(imgpath:str, fovv:float, fovh:float):
  assert os.path.isfile(imgpath), imgpath
  iendsw = '_RGB.png'
  assert imgpath.endswith(iendsw), imgpath
  jsonpath = imgpath[:-len(iendsw)]+'_meta.json'
  assert os.path.isfile(jsonpath), jsonpath
  with open(jsonpath,'r') as infile:
    jdict = json.load(infile)
  assert isinstance(jdict,dict), f'{type(jdict)} -- {jsonpath}'
  imgarr = np.asarray(Image.open(imgpath))
  imww, imhh = imgarr.shape[1], imgarr.shape[0]

  matdictkey = 'extrinsic_cam2world'
  assert matdictkey in jdict, f'{sorted(list(jdict.keys()))} -- {jsonpath}'
  assert len(jdict[matdictkey]) == 12, str(len(jdict[matdictkey]))+' -- '+str(jsonpath)

  if 'fov_v_degrees' in jdict:
    fovv = jdict['fov_v_degrees']; fovh = -999.
  elif 'fov_h_degrees' in jdict:
    fovh = jdict['fov_h_degrees']; fovv = -999.
  fov_v, fov_h = fovv_and_fovh_degrees_given_either(fovv, fovh, imww/imhh)
  fx = focallengthperpixel_fromfovdegrees(fov_h)
  fy = focallengthperpixel_fromfovdegrees(fov_v)
  screenwhf = math.sqrt(float(imww) * float(imhh))
  fxy_pix = screenwhf * math.sqrt(fx*fy)

  extrinsic_cam2world = np.float64(jdict[matdictkey]).reshape((3,4))
  extrinsic_cam2world = np.pad(extrinsic_cam2world, ((0,1),(0,0)))
  extrinsic_cam2world[3,3] = 1.

  # change coordinate system from game (character looks down y axis; z points up)
  # to nerf/colmap (X axis points to the right, the Y axis to the bottom, and the Z axis to the front as seen from the image)
  fixcoords = np.float64([[ 1, 0, 0, 0],
                          [ 0, 0,-1, 0],
                          [ 0, 1, 0, 0],
                          [ 0, 0, 0, 1]])
  colmap_cam2world = np.matmul(extrinsic_cam2world, fixcoords)

  return {"file_path":os.path.join(os.path.basename(os.path.dirname(os.path.abspath(imgpath))), os.path.basename(imgpath)),
    "sharpness": sharpness(imgarr),
    "transform_matrix": colmap_cam2world,
    }, {
      "camera_angle_x": math.radians(fov_h),
      "camera_angle_y": math.radians(fov_v),
      "fl_x": fxy_pix,
      "fl_y": fxy_pix,
      "k1": 0,
      "k2": 0,
      "p1": 0,
      "p2": 0,
      "cx": imww/2,
      "cy": imhh/2,
      "w": imww,
      "h": imhh,
    }


if __name__ == '__main__':
  parser = argparse.ArgumentParser()
  parser.add_argument("-fovv", "--fov_degrees_vertical", type=float, default=-999.0, help="provide if game snapshots didnt contain")
  parser.add_argument("-fovh", "--fov_degrees_horizontal", type=float, default=-999.0, help="provide if game snapshots didnt contain")
  parser.add_argument("--aabb_scale", type=int, default=4, help="Nerf parameter. Must be power of 2. Larger numbers are for large scenes with background stuff in the distance.")
  parser.add_argument("imagefiles", nargs="+", help="next to each image should be a _meta.json")
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

  intrinsics = {}
  camtransfperframe = []
  centroid = []
  for (camtransf, camheader) in process_map(partial(convert_image_with_cam_json_to_nerf_transform,
    fovv=args.fov_degrees_vertical, fovh=args.fov_degrees_horizontal), imagefiles):
    centroid.append(camtransf['transform_matrix'][:3,3])
    camtransfperframe.append(camtransf)
    for kk,vv in camheader.items():
      if kk not in intrinsics:
        intrinsics[kk] = []
      intrinsics[kk].append(float(vv))

  # Centroid of nerf: simply the centroid of the camera locations.
  # Nvidia's colmap2nerf does a fancier calculation
  centroid = np.stack(centroid)
  scale = np.std(centroid, axis=0).max()
  centroid = np.mean(centroid, axis=0)
  if abs(scale - 1.0) < 0.3:
    scale = 1.0
  print("centroid "+str(centroid)+", scale "+str(scale))

  for ii in range(len(camtransfperframe)):
    camtransfperframe[ii]['transform_matrix'][:3,3] -= centroid
    camtransfperframe[ii]['transform_matrix'] = camtransfperframe[ii]['transform_matrix'].tolist()

  # write one intrinsic which is the mean of snapshots... assumes all images taken with same field of view!
  transformjson = {kk:sum(vv)/len(vv) for kk,vv in intrinsics.items()}
  if abs(scale - 1.0) > 0.01:
    transformjson['scale'] = 1.0 / float(scale)
  transformjson['aabb_scale'] = args.aabb_scale
  transformjson['frames'] = camtransfperframe

  with open('transforms.json', 'w') as outfile:
    json.dump(transformjson, outfile, indent=1)
