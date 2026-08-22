#include "render/scene_render_settings.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <string>
#include <string_view>

namespace wide_eye::render {
namespace {

[[nodiscard]] bool all_finite(const std::array<float, 3>& value) {
    return std::isfinite(value[0]) && std::isfinite(value[1]) && std::isfinite(value[2]);
}

[[nodiscard]] std::string format_glsl_vec3(const std::array<float, 3>& value) {
    return "vec3(" + format_glsl_float(value[0]) + ", " + format_glsl_float(value[1]) + ", " +
           format_glsl_float(value[2]) + ")";
}

[[nodiscard]] std::string glsl_float_constant(std::string_view name, float value) {
    return "const float " + std::string{name} + " = " + format_glsl_float(value) + ";\n";
}

[[nodiscard]] std::string glsl_vec3_constant(std::string_view name,
                                             const std::array<float, 3>& value) {
    return "const vec3 " + std::string{name} + " = " + format_glsl_vec3(value) + ";\n";
}

} // namespace

SceneRenderSettingsRejection validate_scene_render_settings(const SceneRenderSettings& settings) {
    if (!std::isfinite(settings.near_plane) || !std::isfinite(settings.far_plane) ||
        !std::isfinite(settings.focal_length) || !all_finite(settings.light_direction) ||
        !all_finite(settings.sun_color) || !all_finite(settings.sky_color) ||
        !std::isfinite(settings.fog_start_distance) ||
        !std::isfinite(settings.fog_end_distance) || !std::isfinite(settings.fog_maximum_blend) ||
        !all_finite(settings.light_projection_center) ||
        !std::isfinite(settings.light_projection_half_extent) ||
        !std::isfinite(settings.light_projection_half_depth)) {
        return SceneRenderSettingsRejection::non_finite_value;
    }
    if (settings.near_plane <= 0.0F) {
        return SceneRenderSettingsRejection::near_plane_not_positive;
    }
    if (settings.far_plane <= settings.near_plane) {
        return SceneRenderSettingsRejection::far_plane_not_beyond_near;
    }
    if (settings.focal_length <= 0.0F) {
        return SceneRenderSettingsRejection::focal_length_not_positive;
    }
    // Exactly the condition under which the shaders' `normalize` produces a
    // finite direction: the squared length computed the way GLSL computes it
    // must be positive and representable.
    const float light_squared_length = settings.light_direction[0] * settings.light_direction[0] +
                                       settings.light_direction[1] * settings.light_direction[1] +
                                       settings.light_direction[2] * settings.light_direction[2];
    if (!std::isfinite(light_squared_length) || light_squared_length <= 0.0F) {
        return SceneRenderSettingsRejection::degenerate_light_direction;
    }
    // `smoothstep` is undefined when its edges are equal and inverted when they
    // are reversed, so both are refused rather than drawn.
    if (settings.fog_end_distance <= settings.fog_start_distance) {
        return SceneRenderSettingsRejection::fog_range_not_increasing;
    }
    if (settings.fog_maximum_blend < 0.0F || settings.fog_maximum_blend > 1.0F) {
        return SceneRenderSettingsRejection::fog_blend_out_of_range;
    }
    // The light-space projection divides by these, so a zero or negative extent
    // is a non-finite or mirrored shadow lookup rather than a small one.
    if (settings.light_projection_half_extent <= 0.0F ||
        settings.light_projection_half_depth <= 0.0F) {
        return SceneRenderSettingsRejection::light_projection_extent_not_positive;
    }
    return SceneRenderSettingsRejection::none;
}

const char* describe(SceneRenderSettingsRejection rejection) {
    switch (rejection) {
    case SceneRenderSettingsRejection::none:
        return "none";
    case SceneRenderSettingsRejection::non_finite_value:
        return "non_finite_value";
    case SceneRenderSettingsRejection::near_plane_not_positive:
        return "near_plane_not_positive";
    case SceneRenderSettingsRejection::far_plane_not_beyond_near:
        return "far_plane_not_beyond_near";
    case SceneRenderSettingsRejection::focal_length_not_positive:
        return "focal_length_not_positive";
    case SceneRenderSettingsRejection::degenerate_light_direction:
        return "degenerate_light_direction";
    case SceneRenderSettingsRejection::fog_range_not_increasing:
        return "fog_range_not_increasing";
    case SceneRenderSettingsRejection::fog_blend_out_of_range:
        return "fog_blend_out_of_range";
    case SceneRenderSettingsRejection::light_projection_extent_not_positive:
        return "light_projection_extent_not_positive";
    }
    return "unknown";
}

std::string format_glsl_float(float value) {
    std::array<char, 64> buffer{};
    const std::to_chars_result result =
        std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    if (result.ec != std::errc{}) {
        // Unreachable for a validated finite float; a caller that reaches it
        // gets a literal that fails to compile rather than a silent zero.
        return "<unrepresentable>";
    }
    std::string text{buffer.data(), static_cast<std::size_t>(result.ptr - buffer.data())};
    // `to_chars` prints the shortest round trip, which drops the decimal point
    // for whole numbers. GLSL would then read an int literal, so put it back.
    if (text.find_first_of(".eE") == std::string::npos) {
        text += ".0";
    }
    return text;
}

std::string compose_scene_shader_preamble(const SceneRenderSettings& settings) {
    std::string preamble =
        "// Generated from wide_eye::render::SceneRenderSettings. A shader body\n"
        "// below must consume these names, never restate their values.\n";
    preamble += glsl_float_constant("near_plane", settings.near_plane);
    preamble += glsl_float_constant("far_plane", settings.far_plane);
    preamble += glsl_float_constant("focal_length", settings.focal_length);
    preamble += glsl_vec3_constant("sun_color", settings.sun_color);
    preamble += glsl_vec3_constant("sky_color", settings.sky_color);
    preamble += glsl_float_constant("fog_start_distance", settings.fog_start_distance);
    preamble += glsl_float_constant("fog_end_distance", settings.fog_end_distance);
    preamble += glsl_float_constant("fog_maximum_blend", settings.fog_maximum_blend);
    // The light basis is derived in shader text rather than in C++ so the
    // driver folds exactly the expression it always folded.
    preamble += "const vec3 light_forward = normalize(" +
                format_glsl_vec3(settings.light_direction) + ");\n";
    preamble += "const vec3 light_right = normalize(cross(light_forward, vec3(0.0, 1.0, 0.0)));\n";
    preamble += "const vec3 light_up = cross(light_right, light_forward);\n";
    preamble += glsl_vec3_constant("light_center", settings.light_projection_center);
    preamble += glsl_float_constant("light_half_extent", settings.light_projection_half_extent);
    preamble += glsl_float_constant("light_half_depth", settings.light_projection_half_depth);
    // One light-space projection: the shadow pass renders with it and both
    // receiving programs sample with it, so they cannot disagree.
    preamble += R"glsl(
vec4 light_clip_position(vec3 position) {
    vec3 offset = position - light_center;
    return vec4(
        dot(offset, light_right) / light_half_extent,
        dot(offset, light_up) / light_half_extent,
        dot(offset, light_forward) / light_half_depth,
        1.0);
}
)glsl";
    return preamble;
}

} // namespace wide_eye::render
