# Copyright (C) 2022 Jason Bunk
import math
import numpy as np


def focallengthperpixel_fromfovdegrees(fov_degrees:float):
    return 0.5/math.tan(float(fov_degrees)*math.pi/360.)


def horizontal_fov_from_vertical_fov_degrees(fov_vertical_degrees:float, screen_aspect_ratio:float):
	fvert = math.tan(float(fov_vertical_degrees)*math.pi/360.)
	return math.atan(fvert*screen_aspect_ratio)*360.0/math.pi

def vertical_fov_from_horizontal_fov_degrees(fov_horizontal_degrees:float, screen_aspect_ratio:float):
    return horizontal_fov_from_vertical_fov_degrees(fov_horizontal_degrees, 1.0/screen_aspect_ratio)


# given either, get both horizontal and vertical fov
def fovv_and_fovh_degrees_given_either(fovv:float, fovh:float, screen_aspect_ratio:float):
    havefov_v = (fovv is not None and fovv > 0. and fovv < 181.)
    havefov_h = (fovh is not None and fovh > 0. and fovh < 181.)
    assert (havefov_v or havefov_h) and not (havefov_v and havefov_h), f'provide one fov: {fovv}, {fovh}'
    if havefov_v:
        return fovv, horizontal_fov_from_vertical_fov_degrees(fovv, screen_aspect_ratio)
    else:
        return vertical_fov_from_horizontal_fov_degrees(fovh, screen_aspect_ratio), fovh


def build_intrinsicmatrix_camtoscreenpix_pinhole_camera(fov_vertical_degrees:float, screen_width:int, screen_height:int):
    fov_horizontal_degrees = horizontal_fov_from_vertical_fov_degrees(fov_vertical_degrees, float(screen_width)/float(screen_height))
    fx = focallengthperpixel_fromfovdegrees(fov_horizontal_degrees)
    fy = focallengthperpixel_fromfovdegrees(fov_vertical_degrees)
    screenwhf = math.sqrt(float(screen_width * screen_height))
    cam2screen = np.zeros((4,4), dtype=np.float64)
    cam2screen[3,3] = 1.
    cam2screen[2,1] = 1.
    cam2screen[0,0] = screenwhf*math.sqrt(fx*fy)
    cam2screen[1,2] = -screenwhf*math.sqrt(fx*fy)
    cam2screen[0,1] = 0.5*float(screen_width)
    cam2screen[1,1] = 0.5*float(screen_height)
    return cam2screen


def depth_image_to_4dscreencolumnvectors(depth):
    mesh = np.array(np.meshgrid(np.arange(depth.shape[1]), np.arange(depth.shape[0]))).reshape((2,-1))
    depth = np.expand_dims(depth.flatten(),axis=0)
    depth = np.concatenate((np.multiply(depth,mesh.astype(depth.dtype)),depth,np.ones_like(depth)), axis=0)
    return depth, mesh.transpose()
