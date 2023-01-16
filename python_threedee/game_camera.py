# Copyright (C) 2022 Jason Bunk
import math

def focallengthperpixel_fromfovdegrees(fov_degrees:float):
    return 0.5/math.tan(float(fov_degrees)*math.pi/360.)

def horizontal_fov_from_vertical_fov_degrees(fov_vertical_degrees:float, screen_aspect_ratio:float):
	fvert = math.tan(float(fov_vertical_degrees)*math.pi/360.)
	return math.atan(fvert*screen_aspect_ratio)*360.0/math.pi
def vertical_fov_from_horizontal_fov_degrees(fov_horizontal_degrees:float, screen_aspect_ratio:float):
    return horizontal_fov_from_vertical_fov_degrees(fov_horizontal_degrees, 1.0/screen_aspect_ratio)
