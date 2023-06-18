#pragma once

namespace my_graphics_api
{
	enum api_enum : int
	{
		d3d9 = 0x9000,
		d3d10 = 0xa000,
		d3d11 = 0xb000,
		d3d12 = 0xc000,
		opengl = 0x10000,
		vulkan = 0x20000
	};
}