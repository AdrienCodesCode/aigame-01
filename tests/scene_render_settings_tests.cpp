#include "render/scene_render_settings.hpp"

#include <array>
#include <bit>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <string>

namespace {

using wide_eye::render::compose_scene_shader_preamble;
using wide_eye::render::describe;
using wide_eye::render::format_glsl_float;
using wide_eye::render::SceneRenderSettings;
using wide_eye::render::SceneRenderSettingsRejection;
using wide_eye::render::validate_scene_render_settings;

int g_failures = 0;

bool check(bool condition, const char* name) {
    if (!condition) {
        std::cerr << "scene_render_settings_failure=" << name << '\n';
        ++g_failures;
    }
    return condition;
}

// Bit equality rather than `==`, so a NaN case is compared honestly and a value
// that merely prints the same cannot pass for the same float.
[[nodiscard]] bool same_float(float left, float right) {
    return std::bit_cast<std::uint32_t>(left) == std::bit_cast<std::uint32_t>(right);
}

[[nodiscard]] std::optional<float> parse_float(const std::string& text, std::size_t& position) {
    while (position < text.size() && (text[position] == ' ' || text[position] == '\n')) {
        ++position;
    }
    float value = 0.0F;
    const std::from_chars_result result =
        std::from_chars(text.data() + position, text.data() + text.size(), value);
    if (result.ec != std::errc{}) {
        return std::nullopt;
    }
    position = static_cast<std::size_t>(result.ptr - text.data());
    return value;
}

// Reads a value back out of the composed GLSL exactly as a shader compiler
// would see it. This is what proves the shader text carries the validated
// settings and not a rounded print of them.
[[nodiscard]] std::optional<float> declared_float(const std::string& text,
                                                  const std::string& name) {
    const std::string prefix = "const float " + name + " = ";
    const std::size_t found = text.find(prefix);
    if (found == std::string::npos) {
        return std::nullopt;
    }
    std::size_t position = found + prefix.size();
    return parse_float(text, position);
}

[[nodiscard]] std::optional<std::array<float, 3>> declared_vec3(const std::string& text,
                                                                const std::string& prefix) {
    const std::size_t found = text.find(prefix);
    if (found == std::string::npos) {
        return std::nullopt;
    }
    std::size_t position = found + prefix.size();
    std::array<float, 3> components{};
    for (std::size_t index = 0; index < components.size(); ++index) {
        const std::optional<float> component = parse_float(text, position);
        if (!component.has_value()) {
            return std::nullopt;
        }
        components[index] = *component;
        if (index + 1U < components.size()) {
            if (position >= text.size() || text[position] != ',') {
                return std::nullopt;
            }
            ++position;
        }
    }
    return components;
}

void rejects(const SceneRenderSettings& settings, SceneRenderSettingsRejection expected,
             const char* name) {
    const SceneRenderSettingsRejection actual = validate_scene_render_settings(settings);
    if (actual != expected) {
        std::cerr << "scene_render_settings_failure=" << name << " expected=" << describe(expected)
                  << " actual=" << describe(actual) << '\n';
        ++g_failures;
    }
}

void check_accepted_values() {
    const SceneRenderSettings settings{};
    // The accepted scene, pinned exactly. A later edit that moves a pixel has to
    // change this test on purpose instead of drifting past it.
    check(same_float(settings.near_plane, 0.1F), "accepted_near_plane");
    check(same_float(settings.far_plane, 100.0F), "accepted_far_plane");
    check(same_float(settings.focal_length, 1.7320508F), "accepted_focal_length");
    check(same_float(settings.light_direction[0], 0.45F) &&
              same_float(settings.light_direction[1], -1.0F) &&
              same_float(settings.light_direction[2], 0.30F),
          "accepted_light_direction");
    check(same_float(settings.sun_color[0], 1.0F) && same_float(settings.sun_color[1], 0.94F) &&
              same_float(settings.sun_color[2], 0.82F),
          "accepted_sun_color");
    check(same_float(settings.sky_color[0], 0.47F) && same_float(settings.sky_color[1], 0.66F) &&
              same_float(settings.sky_color[2], 0.82F),
          "accepted_sky_color");
    check(same_float(settings.fog_start_distance, 36.0F), "accepted_fog_start_distance");
    check(same_float(settings.fog_end_distance, 70.0F), "accepted_fog_end_distance");
    check(same_float(settings.fog_maximum_blend, 0.58F), "accepted_fog_maximum_blend");
    check(same_float(settings.light_projection_center[0], 16.0F) &&
              same_float(settings.light_projection_center[1], 4.0F) &&
              same_float(settings.light_projection_center[2], 16.0F),
          "accepted_light_projection_center");
    check(same_float(settings.light_projection_half_extent, 30.0F),
          "accepted_light_projection_half_extent");
    check(same_float(settings.light_projection_half_depth, 32.0F),
          "accepted_light_projection_half_depth");
    check(validate_scene_render_settings(settings) == SceneRenderSettingsRejection::none,
          "accepted_settings_validate");
}

void check_literal_round_trip() {
    // Every literal the preamble can emit has to read back as the identical
    // float; otherwise composing shader text would quietly move the image.
    const std::array<float, 14> values{0.1F,   100.0F, 1.7320508F, 0.45F, -1.0F,
                                       0.30F,  0.94F,  0.82F,      0.47F, 0.66F,
                                       36.0F,  70.0F,  0.58F,      16.0F};
    for (const float value : values) {
        const std::string text = format_glsl_float(value);
        float parsed = 0.0F;
        const std::from_chars_result result =
            std::from_chars(text.data(), text.data() + text.size(), parsed);
        const bool exact = result.ec == std::errc{} &&
                           result.ptr == text.data() + text.size() && same_float(parsed, value);
        // GLSL reads a digit sequence with no point or exponent as an int.
        const bool reads_as_float = text.find_first_of(".eE") != std::string::npos;
        if (!check(exact, "literal_round_trips") || !check(reads_as_float, "literal_is_float")) {
            std::cerr << "scene_render_settings_literal=" << text << '\n';
        }
    }
    check(format_glsl_float(100.0F) == "100.0", "whole_number_keeps_decimal_point");
}

void check_preamble_carries_settings() {
    const SceneRenderSettings settings{};
    const std::string preamble = compose_scene_shader_preamble(settings);

    // The renderer owns the version directive; a preamble that carried one
    // could not be placed after it.
    check(preamble.find("#version") == std::string::npos, "preamble_has_no_version_directive");

    const std::optional<float> near_plane = declared_float(preamble, "near_plane");
    const std::optional<float> far_plane = declared_float(preamble, "far_plane");
    const std::optional<float> focal_length = declared_float(preamble, "focal_length");
    const std::optional<float> fog_start = declared_float(preamble, "fog_start_distance");
    const std::optional<float> fog_end = declared_float(preamble, "fog_end_distance");
    const std::optional<float> fog_blend = declared_float(preamble, "fog_maximum_blend");
    const std::optional<float> half_extent = declared_float(preamble, "light_half_extent");
    const std::optional<float> half_depth = declared_float(preamble, "light_half_depth");
    const std::optional<std::array<float, 3>> sun_color =
        declared_vec3(preamble, "const vec3 sun_color = vec3(");
    const std::optional<std::array<float, 3>> sky_color =
        declared_vec3(preamble, "const vec3 sky_color = vec3(");
    const std::optional<std::array<float, 3>> light_center =
        declared_vec3(preamble, "const vec3 light_center = vec3(");
    const std::optional<std::array<float, 3>> light_direction =
        declared_vec3(preamble, "const vec3 light_forward = normalize(vec3(");

    if (!check(near_plane.has_value() && far_plane.has_value() && focal_length.has_value() &&
                   fog_start.has_value() && fog_end.has_value() && fog_blend.has_value() &&
                   half_extent.has_value() && half_depth.has_value() && sun_color.has_value() &&
                   sky_color.has_value() && light_center.has_value() &&
                   light_direction.has_value(),
               "preamble_declares_every_setting")) {
        std::cerr << preamble << '\n';
        return;
    }

    check(same_float(*near_plane, settings.near_plane), "preamble_near_plane");
    check(same_float(*far_plane, settings.far_plane), "preamble_far_plane");
    check(same_float(*focal_length, settings.focal_length), "preamble_focal_length");
    check(same_float(*fog_start, settings.fog_start_distance), "preamble_fog_start_distance");
    check(same_float(*fog_end, settings.fog_end_distance), "preamble_fog_end_distance");
    check(same_float(*fog_blend, settings.fog_maximum_blend), "preamble_fog_maximum_blend");
    check(same_float(*half_extent, settings.light_projection_half_extent),
          "preamble_light_half_extent");
    check(same_float(*half_depth, settings.light_projection_half_depth),
          "preamble_light_half_depth");
    for (std::size_t index = 0; index < 3U; ++index) {
        check(same_float((*sun_color)[index], settings.sun_color[index]), "preamble_sun_color");
        check(same_float((*sky_color)[index], settings.sky_color[index]), "preamble_sky_color");
        check(same_float((*light_center)[index], settings.light_projection_center[index]),
              "preamble_light_center");
        check(same_float((*light_direction)[index], settings.light_direction[index]),
              "preamble_light_direction");
    }

    // The one light-space projection the shadow pass and both receivers share.
    check(preamble.find("vec4 light_clip_position(vec3 position)") != std::string::npos,
          "preamble_declares_light_clip_position");
    // The light basis stays a shader-side derivation so the driver folds the
    // same expression it always folded.
    check(preamble.find("normalize(cross(light_forward, vec3(0.0, 1.0, 0.0)))") !=
              std::string::npos,
          "preamble_derives_light_basis_in_glsl");
}

void check_rejections() {
    const SceneRenderSettings accepted{};

    for (const float bad : {std::numeric_limits<float>::quiet_NaN(),
                            std::numeric_limits<float>::infinity()}) {
        SceneRenderSettings settings = accepted;
        settings.focal_length = bad;
        rejects(settings, SceneRenderSettingsRejection::non_finite_value,
                "rejects_non_finite_scalar");
        settings = accepted;
        settings.sky_color[1] = bad;
        rejects(settings, SceneRenderSettingsRejection::non_finite_value,
                "rejects_non_finite_color");
        settings = accepted;
        settings.light_projection_center[2] = bad;
        rejects(settings, SceneRenderSettingsRejection::non_finite_value,
                "rejects_non_finite_light_center");
        settings = accepted;
        settings.fog_end_distance = bad;
        rejects(settings, SceneRenderSettingsRejection::non_finite_value, "rejects_non_finite_fog");
    }

    {
        SceneRenderSettings settings = accepted;
        settings.near_plane = 0.0F;
        rejects(settings, SceneRenderSettingsRejection::near_plane_not_positive,
                "rejects_zero_near_plane");
        settings.near_plane = -0.1F;
        rejects(settings, SceneRenderSettingsRejection::near_plane_not_positive,
                "rejects_negative_near_plane");
    }
    {
        SceneRenderSettings settings = accepted;
        settings.far_plane = settings.near_plane;
        rejects(settings, SceneRenderSettingsRejection::far_plane_not_beyond_near,
                "rejects_equal_far_plane");
        settings.far_plane = settings.near_plane * 0.5F;
        rejects(settings, SceneRenderSettingsRejection::far_plane_not_beyond_near,
                "rejects_inverted_depth_range");
    }
    {
        SceneRenderSettings settings = accepted;
        settings.focal_length = 0.0F;
        rejects(settings, SceneRenderSettingsRejection::focal_length_not_positive,
                "rejects_zero_focal_length");
    }
    {
        SceneRenderSettings settings = accepted;
        settings.light_direction = {0.0F, 0.0F, 0.0F};
        rejects(settings, SceneRenderSettingsRejection::degenerate_light_direction,
                "rejects_zero_length_light_direction");
        // -0.0 is not zero under `<= 0.0F` on the components, but its squared
        // length still is; the check is on the length the shader normalizes.
        settings.light_direction = {-0.0F, 0.0F, -0.0F};
        rejects(settings, SceneRenderSettingsRejection::degenerate_light_direction,
                "rejects_negative_zero_light_direction");
        // A direction whose squared length overflows is what `normalize` turns
        // into a NaN, so it is refused with the same reason.
        settings.light_direction = {std::numeric_limits<float>::max(), 0.0F, 0.0F};
        rejects(settings, SceneRenderSettingsRejection::degenerate_light_direction,
                "rejects_unnormalizable_light_direction");
    }
    {
        SceneRenderSettings settings = accepted;
        settings.fog_start_distance = 70.0F;
        settings.fog_end_distance = 36.0F;
        rejects(settings, SceneRenderSettingsRejection::fog_range_not_increasing,
                "rejects_inverted_fog_range");
        settings.fog_start_distance = 36.0F;
        settings.fog_end_distance = 36.0F;
        rejects(settings, SceneRenderSettingsRejection::fog_range_not_increasing,
                "rejects_degenerate_fog_range");
    }
    {
        SceneRenderSettings settings = accepted;
        settings.fog_maximum_blend = 1.5F;
        rejects(settings, SceneRenderSettingsRejection::fog_blend_out_of_range,
                "rejects_fog_blend_above_one");
        settings.fog_maximum_blend = -0.01F;
        rejects(settings, SceneRenderSettingsRejection::fog_blend_out_of_range,
                "rejects_negative_fog_blend");
    }
    {
        SceneRenderSettings settings = accepted;
        settings.light_projection_half_extent = 0.0F;
        rejects(settings, SceneRenderSettingsRejection::light_projection_extent_not_positive,
                "rejects_zero_light_projection_extent");
        settings = accepted;
        settings.light_projection_half_depth = -32.0F;
        rejects(settings, SceneRenderSettingsRejection::light_projection_extent_not_positive,
                "rejects_negative_light_projection_depth");
    }
}

} // namespace

int main() {
    check_accepted_values();
    check_literal_round_trip();
    check_preamble_carries_settings();
    check_rejections();

    if (g_failures != 0) {
        std::cerr << "scene_render_settings_failures=" << g_failures << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "scene_render_settings_preamble_bytes="
              << compose_scene_shader_preamble(SceneRenderSettings{}).size() << '\n'
              << "scene_render_settings_result=pass\n";
    return EXIT_SUCCESS;
}
