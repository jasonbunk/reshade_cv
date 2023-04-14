In this folder is nearly a complete reshade addon.

One would just need to include "render_target_stats_tracking.hpp" and register stats tracking with ```register_rgb_render_target_stats_tracking();```,
and use the function ```imgui_draw_rgb_render_target_stats_in_reshade_overlay()``` to show the found render targets in the reshade imgui overlay.

Then, clicked buffers will be available in the struct in "clicked_rgb_rendertargets.hpp".
