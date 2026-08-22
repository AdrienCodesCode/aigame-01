#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace wide_eye::render {

// The one source for the values the scene programs used to restate: the
// perspective range, the directional light and its colour, the sky and the
// distance fog that blends toward it, and the orthographic projection the
// single shadow map is rendered with. Each of those numbers used to be written
// out again inside every GLSL source that needed it — the perspective range in
// five vertex shaders, the light direction in six shaders, the shadow
// projection in three — so the shadow pass and the two programs that sample it
// could drift apart without anything failing.
//
// This is deliberately not a general render-settings framework. It holds the
// constants the current paddock, sheep, dog, and debug-line programs actually
// repeated, and nothing else. It also holds no OpenGL type, so nothing in this
// header obliges a caller to have a context.
struct SceneRenderSettings {
    // Perspective range shared by every scene vertex program. `focal_length`
    // is cot(vertical_field_of_view / 2); 1.7320508 is a 60 degree vertical
    // field of view.
    float near_plane = 0.1F;
    float far_plane = 100.0F;
    float focal_length = 1.7320508F;

    // Directional light. `light_direction` points from the sun toward the
    // ground and is normalized where it is used, so its length carries no
    // meaning beyond having to be normalizable.
    std::array<float, 3> light_direction{0.45F, -1.0F, 0.30F};
    std::array<float, 3> sun_color{1.0F, 0.94F, 0.82F};

    // The sky is both the colour the frame is cleared to and the colour
    // distance fog blends toward, which is why one value serves both.
    std::array<float, 3> sky_color{0.47F, 0.66F, 0.82F};
    float fog_start_distance = 36.0F;
    float fog_end_distance = 70.0F;
    float fog_maximum_blend = 0.58F;

    // Orthographic light-space projection for the single shadow map. The half
    // extents are the divisors that map light-space offsets into clip space.
    std::array<float, 3> light_projection_center{16.0F, 4.0F, 16.0F};
    float light_projection_half_extent = 30.0F;
    float light_projection_half_depth = 32.0F;
};

// Why a settings record was rejected. Each reason names something that cannot
// be drawn rather than something merely unusual, so a caller that reads this
// enumeration is reading an actual failure.
enum class SceneRenderSettingsRejection : std::uint8_t {
    none,
    non_finite_value,
    near_plane_not_positive,
    far_plane_not_beyond_near,
    focal_length_not_positive,
    degenerate_light_direction,
    fog_range_not_increasing,
    fog_blend_out_of_range,
    light_projection_extent_not_positive,
};

[[nodiscard]] SceneRenderSettingsRejection
validate_scene_render_settings(const SceneRenderSettings& settings);

// A stable snake_case token for a rejection, for diagnostics and tests.
[[nodiscard]] const char* describe(SceneRenderSettingsRejection rejection);

// The shortest decimal spelling of `value` that reads back as the identical
// float, with a decimal point forced so GLSL parses it as a float rather than
// an int. This round trip is what makes the composed shader text carry exactly
// the settings the renderer validated.
[[nodiscard]] std::string format_glsl_float(float value);

// The GLSL declaration block every scene program is compiled with. It is the
// only place these values are written into shader text; a shader body that
// restates one of them has reintroduced the duplication this replaced. The
// block carries no `#version` directive — the renderer owns that line.
[[nodiscard]] std::string compose_scene_shader_preamble(const SceneRenderSettings& settings);

} // namespace wide_eye::render
