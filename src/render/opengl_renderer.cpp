#include "render/opengl_renderer.hpp"

#include "voxel/handcrafted_paddock.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <glad/gl.h>
#include <limits>
#include <ostream>
#include <type_traits>
#include <vector>

namespace wide_eye::render {
namespace {

constexpr char kTriangleVertexShaderSource[] = R"glsl(#version 460 core
layout(location = 0) in vec2 in_position;
layout(location = 1) in vec3 in_color;

out vec3 vertex_color;

void main() {
    vertex_color = in_color;
    gl_Position = vec4(in_position, 0.0, 1.0);
}
)glsl";

constexpr char kVoxelCubeVertexShaderSource[] = R"glsl(#version 460 core
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;

uniform float aspect_ratio;

out vec3 vertex_color;

void main() {
    const float cos_y = 0.8660254;
    const float sin_y = 0.5;
    const float cos_x = 0.9396926;
    const float sin_x = -0.3420201;

    vec3 rotated_y = vec3(
        cos_y * in_position.x + sin_y * in_position.z,
        in_position.y,
        -sin_y * in_position.x + cos_y * in_position.z);
    vec3 camera_position = vec3(
        rotated_y.x,
        cos_x * rotated_y.y - sin_x * rotated_y.z,
        sin_x * rotated_y.y + cos_x * rotated_y.z - 3.0);

    const float near_plane = 0.1;
    const float far_plane = 100.0;
    const float focal_length = 1.7320508;
    float projection_z = ((far_plane + near_plane) / (near_plane - far_plane)) *
                             camera_position.z +
                         ((2.0 * far_plane * near_plane) / (near_plane - far_plane));

    vertex_color = in_color;
    gl_Position = vec4(
        (focal_length / max(aspect_ratio, 0.0001)) * camera_position.x,
        focal_length * camera_position.y,
        projection_z,
        -camera_position.z);
}
)glsl";

constexpr char kHandcraftedPaddockVertexShaderSource[] = R"glsl(#version 460 core
layout(location = 0) in vec3 in_position;
layout(location = 1) in ivec3 in_normal;
layout(location = 2) in uint in_material;

uniform float aspect_ratio;
uniform vec3 camera_eye;
uniform vec3 camera_target;

out vec3 world_position;
out vec3 world_normal;
flat out uint material_id;
out vec4 shadow_position;

vec4 light_clip_position(vec3 position) {
    const vec3 light_forward = normalize(vec3(0.45, -1.0, 0.30));
    const vec3 light_right = normalize(cross(light_forward, vec3(0.0, 1.0, 0.0)));
    const vec3 light_up = cross(light_right, light_forward);
    const vec3 light_center = vec3(16.0, 4.0, 16.0);
    vec3 offset = position - light_center;
    return vec4(
        dot(offset, light_right) / 30.0,
        dot(offset, light_up) / 30.0,
        dot(offset, light_forward) / 32.0,
        1.0);
}

void main() {
    const vec3 world_up = vec3(0.0, 1.0, 0.0);
    vec3 forward = normalize(camera_target - camera_eye);
    vec3 right = normalize(cross(forward, world_up));
    vec3 up = cross(right, forward);
    vec3 relative = in_position - camera_eye;
    vec3 camera_position = vec3(
        dot(relative, right),
        dot(relative, up),
        -dot(relative, forward));

    const float near_plane = 0.1;
    const float far_plane = 100.0;
    const float focal_length = 1.7320508;
    float projection_z = ((far_plane + near_plane) / (near_plane - far_plane)) *
                             camera_position.z +
                         ((2.0 * far_plane * near_plane) / (near_plane - far_plane));

    world_position = in_position;
    world_normal = vec3(in_normal);
    material_id = in_material;
    shadow_position = light_clip_position(in_position);
    gl_Position = vec4(
        (focal_length / max(aspect_ratio, 0.0001)) * camera_position.x,
        focal_length * camera_position.y,
        projection_z,
        -camera_position.z);
}
)glsl";

constexpr char kHandcraftedPaddockFragmentShaderSource[] = R"glsl(#version 460 core
in vec3 world_position;
in vec3 world_normal;
flat in uint material_id;
in vec4 shadow_position;

uniform vec3 material_palette[7];
uniform sampler2D shadow_map;
uniform int debug_mode;
uniform vec3 camera_eye;

layout(location = 0) out vec4 fragment_color;

float shadow_visibility(vec3 normal) {
    vec3 projected = shadow_position.xyz / shadow_position.w * 0.5 + 0.5;
    if (projected.x <= 0.0 || projected.x >= 1.0 || projected.y <= 0.0 ||
        projected.y >= 1.0 || projected.z <= 0.0 || projected.z >= 1.0) {
        return 1.0;
    }

    const vec3 light_forward = normalize(vec3(0.45, -1.0, 0.30));
    float normal_light = max(dot(normal, -light_forward), 0.0);
    float bias = max(0.0015 * (1.0 - normal_light), 0.00045);
    vec2 texel = 1.0 / vec2(textureSize(shadow_map, 0));
    float lit_samples = 0.0;
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            float nearest_depth = texture(shadow_map, projected.xy + vec2(x, y) * texel).r;
            lit_samples += projected.z - bias <= nearest_depth ? 1.0 : 0.0;
        }
    }
    return lit_samples / 9.0;
}

void main() {
    if (debug_mode == 1) {
        fragment_color = vec4(1.0, 0.82, 0.12, 1.0);
        return;
    }

    vec3 normal = normalize(world_normal);
    if (debug_mode == 2) {
        fragment_color = vec4(normal * 0.5 + 0.5, 1.0);
        return;
    }

    const vec3 light_forward = normalize(vec3(0.45, -1.0, 0.30));
    const vec3 sun_color = vec3(1.0, 0.94, 0.82);
    const vec3 sky_color = vec3(0.47, 0.66, 0.82);
    vec3 base_color = material_id < 7u ? material_palette[material_id]
                                      : vec3(1.0, 0.0, 1.0);
    float diffuse = max(dot(normal, -light_forward), 0.0);
    float visibility = shadow_visibility(normal);
    float light_amount = 0.42 + diffuse * (0.58 * mix(0.42, 1.0, visibility));
    vec3 lit_color = base_color * sun_color * light_amount;

    float camera_distance = distance(camera_eye, world_position);
    float fog_amount = smoothstep(36.0, 70.0, camera_distance);
    vec3 final_color = mix(lit_color, sky_color, fog_amount * 0.58);
    fragment_color = vec4(final_color, 1.0);
}
)glsl";

constexpr char kHandcraftedPaddockShadowVertexShaderSource[] = R"glsl(#version 460 core
layout(location = 0) in vec3 in_position;

void main() {
    const vec3 light_forward = normalize(vec3(0.45, -1.0, 0.30));
    const vec3 light_right = normalize(cross(light_forward, vec3(0.0, 1.0, 0.0)));
    const vec3 light_up = cross(light_right, light_forward);
    const vec3 light_center = vec3(16.0, 4.0, 16.0);
    vec3 offset = in_position - light_center;
    gl_Position = vec4(
        dot(offset, light_right) / 30.0,
        dot(offset, light_up) / 30.0,
        dot(offset, light_forward) / 32.0,
        1.0);
}
)glsl";

constexpr char kDepthOnlyFragmentShaderSource[] = R"glsl(#version 460 core
void main() {
}
)glsl";

constexpr char kPaddockDebugLineVertexShaderSource[] = R"glsl(#version 460 core
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;

uniform float aspect_ratio;
uniform vec3 camera_eye;
uniform vec3 camera_target;

out vec3 vertex_color;

void main() {
    const vec3 world_up = vec3(0.0, 1.0, 0.0);
    vec3 forward = normalize(camera_target - camera_eye);
    vec3 right = normalize(cross(forward, world_up));
    vec3 up = cross(right, forward);
    vec3 relative = in_position - camera_eye;
    vec3 camera_position = vec3(
        dot(relative, right),
        dot(relative, up),
        -dot(relative, forward));

    const float near_plane = 0.1;
    const float far_plane = 100.0;
    const float focal_length = 1.7320508;
    float projection_z = ((far_plane + near_plane) / (near_plane - far_plane)) *
                             camera_position.z +
                         ((2.0 * far_plane * near_plane) / (near_plane - far_plane));

    vertex_color = in_color;
    gl_Position = vec4(
        (focal_length / max(aspect_ratio, 0.0001)) * camera_position.x,
        focal_length * camera_position.y,
        projection_z,
        -camera_position.z);
}
)glsl";

constexpr char kDogVertexShaderSource[] = R"glsl(#version 460 core
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;

uniform float aspect_ratio;
uniform vec3 camera_eye;
uniform vec3 camera_target;
uniform vec3 dog_position;
uniform float dog_heading;

out vec3 vertex_color;

void main() {
    float cosine = cos(dog_heading);
    float sine = sin(dog_heading);
    vec3 rotated = vec3(
        cosine * in_position.x - sine * in_position.z,
        in_position.y,
        sine * in_position.x + cosine * in_position.z);
    vec3 world_position = dog_position + rotated;

    const vec3 world_up = vec3(0.0, 1.0, 0.0);
    vec3 forward = normalize(camera_target - camera_eye);
    vec3 right = normalize(cross(forward, world_up));
    vec3 up = cross(right, forward);
    vec3 relative = world_position - camera_eye;
    vec3 camera_position = vec3(
        dot(relative, right),
        dot(relative, up),
        -dot(relative, forward));

    const float near_plane = 0.1;
    const float far_plane = 100.0;
    const float focal_length = 1.7320508;
    float projection_z = ((far_plane + near_plane) / (near_plane - far_plane)) *
                             camera_position.z +
                         ((2.0 * far_plane * near_plane) / (near_plane - far_plane));

    vertex_color = in_color;
    gl_Position = vec4(
        (focal_length / max(aspect_ratio, 0.0001)) * camera_position.x,
        focal_length * camera_position.y,
        projection_z,
        -camera_position.z);
}
)glsl";

constexpr char kSheepVertexShaderSource[] = R"glsl(#version 460 core
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_color;

uniform float aspect_ratio;
uniform vec3 camera_eye;
uniform vec3 camera_target;
uniform vec3 sheep_position;
uniform float sheep_heading;
uniform uint sheep_id;

out vec3 world_position;
out vec3 world_normal;
out vec3 vertex_color;
flat out uint proxy_id;
out vec4 shadow_position;

vec4 light_clip_position(vec3 position) {
    const vec3 light_forward = normalize(vec3(0.45, -1.0, 0.30));
    const vec3 light_right = normalize(cross(light_forward, vec3(0.0, 1.0, 0.0)));
    const vec3 light_up = cross(light_right, light_forward);
    const vec3 light_center = vec3(16.0, 4.0, 16.0);
    vec3 offset = position - light_center;
    return vec4(
        dot(offset, light_right) / 30.0,
        dot(offset, light_up) / 30.0,
        dot(offset, light_forward) / 32.0,
        1.0);
}

void main() {
    float cosine = cos(sheep_heading);
    float sine = sin(sheep_heading);
    vec3 rotated_position = vec3(
        cosine * in_position.x - sine * in_position.z,
        in_position.y,
        sine * in_position.x + cosine * in_position.z);
    world_position = sheep_position + rotated_position;
    world_normal = vec3(
        cosine * in_normal.x - sine * in_normal.z,
        in_normal.y,
        sine * in_normal.x + cosine * in_normal.z);
    vertex_color = in_color;
    proxy_id = sheep_id;
    shadow_position = light_clip_position(world_position);

    const vec3 world_up = vec3(0.0, 1.0, 0.0);
    vec3 forward = normalize(camera_target - camera_eye);
    vec3 right = normalize(cross(forward, world_up));
    vec3 up = cross(right, forward);
    vec3 relative = world_position - camera_eye;
    vec3 camera_position = vec3(
        dot(relative, right),
        dot(relative, up),
        -dot(relative, forward));

    const float near_plane = 0.1;
    const float far_plane = 100.0;
    const float focal_length = 1.7320508;
    float projection_z = ((far_plane + near_plane) / (near_plane - far_plane)) *
                             camera_position.z +
                         ((2.0 * far_plane * near_plane) / (near_plane - far_plane));
    gl_Position = vec4(
        (focal_length / max(aspect_ratio, 0.0001)) * camera_position.x,
        focal_length * camera_position.y,
        projection_z,
        -camera_position.z);
}
)glsl";

constexpr char kSheepFragmentShaderSource[] = R"glsl(#version 460 core
in vec3 world_position;
in vec3 world_normal;
in vec3 vertex_color;
flat in uint proxy_id;
in vec4 shadow_position;

uniform sampler2D shadow_map;
uniform vec3 camera_eye;

layout(location = 0) out vec4 fragment_color;

float shadow_visibility(vec3 normal) {
    vec3 projected = shadow_position.xyz / shadow_position.w * 0.5 + 0.5;
    if (projected.x <= 0.0 || projected.x >= 1.0 || projected.y <= 0.0 ||
        projected.y >= 1.0 || projected.z <= 0.0 || projected.z >= 1.0) {
        return 1.0;
    }
    const vec3 light_forward = normalize(vec3(0.45, -1.0, 0.30));
    float normal_light = max(dot(normal, -light_forward), 0.0);
    float bias = max(0.0015 * (1.0 - normal_light), 0.00045);
    vec2 texel = 1.0 / vec2(textureSize(shadow_map, 0));
    float lit_samples = 0.0;
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            float nearest_depth = texture(shadow_map, projected.xy + vec2(x, y) * texel).r;
            lit_samples += projected.z - bias <= nearest_depth ? 1.0 : 0.0;
        }
    }
    return lit_samples / 9.0;
}

void main() {
    vec3 normal = normalize(world_normal);
    const vec3 light_forward = normalize(vec3(0.45, -1.0, 0.30));
    const vec3 sun_color = vec3(1.0, 0.94, 0.82);
    const vec3 sky_color = vec3(0.47, 0.66, 0.82);
    float diffuse = max(dot(normal, -light_forward), 0.0);
    float visibility = shadow_visibility(normal);
    float light_amount = 0.46 + diffuse * (0.54 * mix(0.42, 1.0, visibility));
    float identity_tint = 0.98 + float(proxy_id % 3u) * 0.01;
    vec3 lit_color = vertex_color * identity_tint * sun_color * light_amount;
    float fog_amount = smoothstep(36.0, 70.0, distance(camera_eye, world_position));
    fragment_color = vec4(mix(lit_color, sky_color, fog_amount * 0.58), 1.0);
}
)glsl";

constexpr char kFragmentShaderSource[] = R"glsl(#version 460 core
in vec3 vertex_color;

layout(location = 0) out vec4 fragment_color;

void main() {
    fragment_color = vec4(vertex_color, 1.0);
}
)glsl";

struct TriangleVertex {
    std::array<float, 2> position;
    std::array<float, 3> color;
};

struct CubeVertex {
    std::array<float, 3> position;
    std::array<float, 3> color;
};

struct DebugLineVertex {
    std::array<float, 3> position;
    std::array<float, 3> color;
};

struct DogVertex {
    std::array<float, 3> position;
    std::array<float, 3> color;
};

struct SheepVertex {
    std::array<float, 3> position;
    std::array<float, 3> normal;
    std::array<float, 3> color;
};

void append_sheep_box(std::vector<SheepVertex>& vertices, std::array<float, 3> minimum,
                      std::array<float, 3> maximum, std::array<float, 3> color) {
    const std::array<std::array<float, 3>, 8> corners{{
        {{minimum[0], minimum[1], minimum[2]}},
        {{maximum[0], minimum[1], minimum[2]}},
        {{maximum[0], maximum[1], minimum[2]}},
        {{minimum[0], maximum[1], minimum[2]}},
        {{minimum[0], minimum[1], maximum[2]}},
        {{maximum[0], minimum[1], maximum[2]}},
        {{maximum[0], maximum[1], maximum[2]}},
        {{minimum[0], maximum[1], maximum[2]}},
    }};
    struct Face {
        std::array<std::size_t, 4> corners;
        std::array<float, 3> normal;
    };
    constexpr std::array<Face, 6> kFaces{{
        {{{0, 3, 2, 1}}, {{0.0F, 0.0F, -1.0F}}},
        {{{5, 6, 7, 4}}, {{0.0F, 0.0F, 1.0F}}},
        {{{0, 4, 7, 3}}, {{-1.0F, 0.0F, 0.0F}}},
        {{{1, 2, 6, 5}}, {{1.0F, 0.0F, 0.0F}}},
        {{{3, 7, 6, 2}}, {{0.0F, 1.0F, 0.0F}}},
        {{{0, 1, 5, 4}}, {{0.0F, -1.0F, 0.0F}}},
    }};
    constexpr std::array<std::size_t, 6> kTriangleOrder{{0, 1, 2, 0, 2, 3}};
    for (const Face& face : kFaces) {
        for (const std::size_t corner : kTriangleOrder) {
            vertices.push_back(
                {.position = corners[face.corners[corner]], .normal = face.normal, .color = color});
        }
    }
}

[[nodiscard]] std::vector<SheepVertex> make_sheep_proxy_vertices() {
    constexpr std::array<float, 3> kWool{0.91F, 0.88F, 0.76F};
    constexpr std::array<float, 3> kFace{0.18F, 0.16F, 0.14F};
    constexpr std::array<float, 3> kLeg{0.12F, 0.105F, 0.09F};
    std::vector<SheepVertex> vertices;
    vertices.reserve(9U * 36U);

    append_sheep_box(vertices, {-0.58F, 0.48F, -0.72F}, {0.58F, 1.38F, 0.76F}, kWool);
    append_sheep_box(vertices, {-0.34F, 0.72F, -1.18F}, {0.34F, 1.26F, -0.68F}, kFace);
    append_sheep_box(vertices, {-0.27F, 1.12F, -1.10F}, {-0.04F, 1.38F, -0.76F}, kFace);
    append_sheep_box(vertices, {0.04F, 1.12F, -1.10F}, {0.27F, 1.38F, -0.76F}, kFace);
    append_sheep_box(vertices, {-0.46F, 0.0F, -0.58F}, {-0.24F, 0.58F, -0.34F}, kLeg);
    append_sheep_box(vertices, {0.24F, 0.0F, -0.58F}, {0.46F, 0.58F, -0.34F}, kLeg);
    append_sheep_box(vertices, {-0.46F, 0.0F, 0.34F}, {-0.24F, 0.58F, 0.58F}, kLeg);
    append_sheep_box(vertices, {0.24F, 0.0F, 0.34F}, {0.46F, 0.58F, 0.58F}, kLeg);
    append_sheep_box(vertices, {-0.20F, 0.88F, 0.72F}, {0.20F, 1.18F, 1.02F}, kWool);
    return vertices;
}

[[nodiscard]] std::vector<DogVertex> make_placeholder_dog_vertices() {
    constexpr int kSegmentCount = 12;
    constexpr float kPi = 3.14159265358979323846F;
    constexpr float kRadius = 0.42F;
    constexpr float kHeight = 1.15F;
    constexpr std::array<float, 3> kBodyColor{0.12F, 0.105F, 0.09F};
    constexpr std::array<float, 3> kTopColor{0.86F, 0.84F, 0.76F};
    constexpr std::array<float, 3> kNoseColor{0.025F, 0.025F, 0.022F};
    constexpr std::array<float, 3> kFacingMarkerColor{1.0F, 0.34F, 0.06F};

    std::vector<DogVertex> vertices;
    vertices.reserve(static_cast<std::size_t>(kSegmentCount) * 12U + 12U);
    const auto append = [&](std::array<float, 3> position, std::array<float, 3> color) {
        vertices.push_back({.position = position, .color = color});
    };
    for (int segment = 0; segment < kSegmentCount; ++segment) {
        const float angle_0 =
            2.0F * kPi * static_cast<float>(segment) / static_cast<float>(kSegmentCount);
        const float angle_1 =
            2.0F * kPi * static_cast<float>(segment + 1) / static_cast<float>(kSegmentCount);
        const std::array<float, 3> bottom_0{kRadius * std::cos(angle_0), 0.0F,
                                            kRadius * std::sin(angle_0)};
        const std::array<float, 3> bottom_1{kRadius * std::cos(angle_1), 0.0F,
                                            kRadius * std::sin(angle_1)};
        const std::array<float, 3> top_0{bottom_0[0], kHeight, bottom_0[2]};
        const std::array<float, 3> top_1{bottom_1[0], kHeight, bottom_1[2]};

        append(bottom_0, kBodyColor);
        append(top_1, kBodyColor);
        append(bottom_1, kBodyColor);
        append(bottom_0, kBodyColor);
        append(top_0, kBodyColor);
        append(top_1, kBodyColor);

        append({0.0F, kHeight, 0.0F}, kTopColor);
        append(top_1, kTopColor);
        append(top_0, kTopColor);
        append({0.0F, 0.0F, 0.0F}, kBodyColor);
        append(bottom_0, kBodyColor);
        append(bottom_1, kBodyColor);
    }

    constexpr std::array<float, 3> kNose{0.0F, 0.72F, -0.86F};
    constexpr std::array<float, 3> kNoseLeft{-0.30F, 0.56F, -0.30F};
    constexpr std::array<float, 3> kNoseRight{0.30F, 0.56F, -0.30F};
    constexpr std::array<float, 3> kNoseTop{0.0F, 1.02F, -0.28F};
    append(kNoseLeft, kNoseColor);
    append(kNoseRight, kNoseColor);
    append(kNose, kNoseColor);
    append(kNoseLeft, kNoseColor);
    append(kNose, kNoseColor);
    append(kNoseTop, kNoseColor);
    append(kNoseRight, kNoseColor);
    append(kNoseTop, kNoseColor);
    append(kNose, kNoseColor);
    append(kNoseLeft, kNoseColor);
    append(kNoseTop, kNoseColor);
    append(kNoseRight, kNoseColor);

    // Keep facing readable from the elevated follow camera even when the nose
    // itself is occluded by the body.
    constexpr float kMarkerHeight = kHeight + 0.006F;
    append({0.0F, kMarkerHeight, -0.36F}, kFacingMarkerColor);
    append({-0.16F, kMarkerHeight, 0.10F}, kFacingMarkerColor);
    append({0.16F, kMarkerHeight, 0.10F}, kFacingMarkerColor);
    return vertices;
}

constexpr int kShadowMapExtent = 1024;
constexpr std::array<float, 3> kSkyColor{0.47F, 0.66F, 0.82F};
constexpr std::array<float, 3> kChunkBoundsColor{0.10F, 0.95F, 0.95F};
constexpr std::array<float, 3> kFaceNormalColor{1.0F, 0.15F, 0.78F};

constexpr std::array<TriangleVertex, 3> kTriangleVertices{{
    {{{-0.75F, -0.65F}}, {{0.95F, 0.20F, 0.15F}}},
    {{{0.75F, -0.65F}}, {{0.20F, 0.85F, 0.35F}}},
    {{{0.0F, 0.75F}}, {{0.20F, 0.45F, 1.0F}}},
}};

constexpr CubeVertex cube_vertex(float x, float y, float z, std::array<float, 3> color) {
    return CubeVertex{.position = {x, y, z}, .color = color};
}

constexpr std::array<float, 3> kFrontColor{0.90F, 0.22F, 0.12F};
constexpr std::array<float, 3> kRightColor{0.20F, 0.72F, 0.32F};
constexpr std::array<float, 3> kTopColor{0.24F, 0.48F, 0.95F};
constexpr std::array<float, 3> kLeftColor{0.92F, 0.72F, 0.18F};
constexpr std::array<float, 3> kBottomColor{0.72F, 0.28F, 0.78F};
constexpr std::array<float, 3> kBackColor{0.18F, 0.78F, 0.82F};

// The nearer front face is submitted first and the farther back face last. The
// center-pixel smoke therefore rejects painter's-order rendering without depth.
constexpr std::array<CubeVertex, 36> kVoxelCubeVertices{{
    cube_vertex(-0.5F, -0.5F, 0.5F, kFrontColor),   cube_vertex(0.5F, -0.5F, 0.5F, kFrontColor),
    cube_vertex(0.5F, 0.5F, 0.5F, kFrontColor),     cube_vertex(-0.5F, -0.5F, 0.5F, kFrontColor),
    cube_vertex(0.5F, 0.5F, 0.5F, kFrontColor),     cube_vertex(-0.5F, 0.5F, 0.5F, kFrontColor),

    cube_vertex(0.5F, -0.5F, 0.5F, kRightColor),    cube_vertex(0.5F, -0.5F, -0.5F, kRightColor),
    cube_vertex(0.5F, 0.5F, -0.5F, kRightColor),    cube_vertex(0.5F, -0.5F, 0.5F, kRightColor),
    cube_vertex(0.5F, 0.5F, -0.5F, kRightColor),    cube_vertex(0.5F, 0.5F, 0.5F, kRightColor),

    cube_vertex(-0.5F, 0.5F, 0.5F, kTopColor),      cube_vertex(0.5F, 0.5F, 0.5F, kTopColor),
    cube_vertex(0.5F, 0.5F, -0.5F, kTopColor),      cube_vertex(-0.5F, 0.5F, 0.5F, kTopColor),
    cube_vertex(0.5F, 0.5F, -0.5F, kTopColor),      cube_vertex(-0.5F, 0.5F, -0.5F, kTopColor),

    cube_vertex(-0.5F, -0.5F, -0.5F, kLeftColor),   cube_vertex(-0.5F, -0.5F, 0.5F, kLeftColor),
    cube_vertex(-0.5F, 0.5F, 0.5F, kLeftColor),     cube_vertex(-0.5F, -0.5F, -0.5F, kLeftColor),
    cube_vertex(-0.5F, 0.5F, 0.5F, kLeftColor),     cube_vertex(-0.5F, 0.5F, -0.5F, kLeftColor),

    cube_vertex(-0.5F, -0.5F, -0.5F, kBottomColor), cube_vertex(0.5F, -0.5F, -0.5F, kBottomColor),
    cube_vertex(0.5F, -0.5F, 0.5F, kBottomColor),   cube_vertex(-0.5F, -0.5F, -0.5F, kBottomColor),
    cube_vertex(0.5F, -0.5F, 0.5F, kBottomColor),   cube_vertex(-0.5F, -0.5F, 0.5F, kBottomColor),

    cube_vertex(0.5F, -0.5F, -0.5F, kBackColor),    cube_vertex(-0.5F, -0.5F, -0.5F, kBackColor),
    cube_vertex(-0.5F, 0.5F, -0.5F, kBackColor),    cube_vertex(0.5F, -0.5F, -0.5F, kBackColor),
    cube_vertex(-0.5F, 0.5F, -0.5F, kBackColor),    cube_vertex(0.5F, 0.5F, -0.5F, kBackColor),
}};

static_assert(sizeof(TriangleVertex) == 5U * sizeof(float));
static_assert(sizeof(CubeVertex) == 6U * sizeof(float));
static_assert(sizeof(DebugLineVertex) == 6U * sizeof(float));
static_assert(sizeof(SheepVertex) == 9U * sizeof(float));
static_assert(std::is_standard_layout_v<voxel::ChunkMeshVertex>);

void append_debug_line(std::vector<DebugLineVertex>& vertices, std::array<float, 3> start,
                       std::array<float, 3> end, std::array<float, 3> color) {
    vertices.push_back({.position = start, .color = color});
    vertices.push_back({.position = end, .color = color});
}

void append_chunk_box(std::vector<DebugLineVertex>& vertices, float minimum_x, float minimum_z,
                      float maximum_x, float maximum_z) {
    constexpr float kMinimumY = 0.02F;
    constexpr float kMaximumY = static_cast<float>(voxel::Chunk::kEdgeLength);
    const std::array<std::array<float, 3>, 8> corners{{
        {{minimum_x, kMinimumY, minimum_z}},
        {{maximum_x, kMinimumY, minimum_z}},
        {{maximum_x, kMinimumY, maximum_z}},
        {{minimum_x, kMinimumY, maximum_z}},
        {{minimum_x, kMaximumY, minimum_z}},
        {{maximum_x, kMaximumY, minimum_z}},
        {{maximum_x, kMaximumY, maximum_z}},
        {{minimum_x, kMaximumY, maximum_z}},
    }};
    constexpr std::array<std::array<std::size_t, 2>, 12> kEdges{{
        {{0, 1}},
        {{1, 2}},
        {{2, 3}},
        {{3, 0}},
        {{4, 5}},
        {{5, 6}},
        {{6, 7}},
        {{7, 4}},
        {{0, 4}},
        {{1, 5}},
        {{2, 6}},
        {{3, 7}},
    }};
    for (const auto& edge : kEdges) {
        append_debug_line(vertices, corners[edge[0]], corners[edge[1]], kChunkBoundsColor);
    }
}

void write_shader_log(GLuint shader, std::ostream& diagnostics) {
    std::array<GLchar, 4096> log{};
    GLsizei written = 0;
    glGetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), &written, log.data());
    diagnostics << "shader_log=";
    if (written > 0) {
        diagnostics.write(log.data(), static_cast<std::streamsize>(written));
    } else {
        diagnostics << "<unavailable>";
    }
    diagnostics << '\n';
}

void write_program_log(GLuint program, std::ostream& diagnostics) {
    std::array<GLchar, 4096> log{};
    GLsizei written = 0;
    glGetProgramInfoLog(program, static_cast<GLsizei>(log.size()), &written, log.data());
    diagnostics << "program_log=";
    if (written > 0) {
        diagnostics.write(log.data(), static_cast<std::streamsize>(written));
    } else {
        diagnostics << "<unavailable>";
    }
    diagnostics << '\n';
}

} // namespace

struct OpenGlRenderer::Impl {
    GLuint triangle_program = 0;
    GLuint triangle_vertex_array = 0;
    GLuint triangle_vertex_buffer = 0;
    GLuint cube_program = 0;
    GLuint cube_vertex_array = 0;
    GLuint cube_vertex_buffer = 0;
    GLint cube_aspect_ratio = -1;
    GLuint paddock_program = 0;
    GLuint paddock_vertex_array = 0;
    GLuint paddock_vertex_buffer = 0;
    GLuint paddock_index_buffer = 0;
    GLuint paddock_shadow_program = 0;
    GLuint paddock_shadow_framebuffer = 0;
    GLuint paddock_shadow_texture = 0;
    GLuint paddock_debug_line_program = 0;
    GLuint paddock_debug_line_vertex_array = 0;
    GLuint paddock_debug_line_vertex_buffer = 0;
    // The influence overlay is rebuilt every frame from a published tick, so it
    // gets its own buffer sized once to the declared segment ceiling and then
    // only ever written with glBufferSubData. No draw can grow it, which is why
    // the frame builder carries a checked ceiling in the first place.
    GLuint influence_debug_vertex_array = 0;
    GLuint influence_debug_vertex_buffer = 0;
    GLint paddock_aspect_ratio = -1;
    GLint paddock_camera_eye = -1;
    GLint paddock_camera_target = -1;
    GLint paddock_palette = -1;
    GLint paddock_shadow_map = -1;
    GLint paddock_debug_mode = -1;
    GLint paddock_debug_line_aspect_ratio = -1;
    GLint paddock_debug_line_camera_eye = -1;
    GLint paddock_debug_line_camera_target = -1;
    GLuint dog_program = 0;
    GLuint dog_vertex_array = 0;
    GLuint dog_vertex_buffer = 0;
    GLint dog_aspect_ratio = -1;
    GLint dog_camera_eye = -1;
    GLint dog_camera_target = -1;
    GLint dog_position = -1;
    GLint dog_heading = -1;
    GLsizei dog_vertex_count = 0;
    GLuint sheep_program = 0;
    GLuint sheep_vertex_array = 0;
    GLuint sheep_vertex_buffer = 0;
    GLint sheep_aspect_ratio = -1;
    GLint sheep_camera_eye = -1;
    GLint sheep_camera_target = -1;
    GLint sheep_position = -1;
    GLint sheep_heading = -1;
    GLint sheep_id = -1;
    GLint sheep_shadow_map = -1;
    GLsizei sheep_vertex_count = 0;
    GLsizei paddock_index_count = 0;
    GLsizei paddock_chunk_bounds_first = 0;
    GLsizei paddock_chunk_bounds_count = 0;
    GLsizei paddock_face_normals_first = 0;
    GLsizei paddock_face_normals_count = 0;
    std::size_t paddock_source_chunk_count = 0;
    std::size_t paddock_occupied_block_count = 0;
    std::size_t paddock_vertex_count = 0;

    ~Impl() {
        if (paddock_shadow_framebuffer != 0) {
            glDeleteFramebuffers(1, &paddock_shadow_framebuffer);
        }
        if (paddock_shadow_texture != 0) {
            glDeleteTextures(1, &paddock_shadow_texture);
        }
        if (paddock_debug_line_vertex_buffer != 0) {
            glDeleteBuffers(1, &paddock_debug_line_vertex_buffer);
        }
        if (influence_debug_vertex_buffer != 0) {
            glDeleteBuffers(1, &influence_debug_vertex_buffer);
        }
        if (dog_vertex_buffer != 0) {
            glDeleteBuffers(1, &dog_vertex_buffer);
        }
        if (sheep_vertex_buffer != 0) {
            glDeleteBuffers(1, &sheep_vertex_buffer);
        }
        if (paddock_index_buffer != 0) {
            glDeleteBuffers(1, &paddock_index_buffer);
        }
        if (paddock_vertex_buffer != 0) {
            glDeleteBuffers(1, &paddock_vertex_buffer);
        }
        if (cube_vertex_buffer != 0) {
            glDeleteBuffers(1, &cube_vertex_buffer);
        }
        if (triangle_vertex_buffer != 0) {
            glDeleteBuffers(1, &triangle_vertex_buffer);
        }
        if (cube_vertex_array != 0) {
            glDeleteVertexArrays(1, &cube_vertex_array);
        }
        if (paddock_debug_line_vertex_array != 0) {
            glDeleteVertexArrays(1, &paddock_debug_line_vertex_array);
        }
        if (influence_debug_vertex_array != 0) {
            glDeleteVertexArrays(1, &influence_debug_vertex_array);
        }
        if (dog_vertex_array != 0) {
            glDeleteVertexArrays(1, &dog_vertex_array);
        }
        if (sheep_vertex_array != 0) {
            glDeleteVertexArrays(1, &sheep_vertex_array);
        }
        if (paddock_vertex_array != 0) {
            glDeleteVertexArrays(1, &paddock_vertex_array);
        }
        if (triangle_vertex_array != 0) {
            glDeleteVertexArrays(1, &triangle_vertex_array);
        }
        if (cube_program != 0) {
            glDeleteProgram(cube_program);
        }
        if (paddock_program != 0) {
            glDeleteProgram(paddock_program);
        }
        if (paddock_shadow_program != 0) {
            glDeleteProgram(paddock_shadow_program);
        }
        if (paddock_debug_line_program != 0) {
            glDeleteProgram(paddock_debug_line_program);
        }
        if (dog_program != 0) {
            glDeleteProgram(dog_program);
        }
        if (sheep_program != 0) {
            glDeleteProgram(sheep_program);
        }
        if (triangle_program != 0) {
            glDeleteProgram(triangle_program);
        }
    }

    [[nodiscard]] GLuint compile(GLenum type, const char* source, std::ostream& diagnostics) const {
        const GLuint shader = glCreateShader(type);
        if (shader == 0) {
            diagnostics << "render_error=shader_create_failed\n";
            return 0;
        }

        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint compiled = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (compiled == GL_TRUE) {
            return shader;
        }

        diagnostics << "render_error=shader_compile_failed type=" << type << '\n';
        write_shader_log(shader, diagnostics);
        glDeleteShader(shader);
        return 0;
    }

    [[nodiscard]] GLuint
    create_program_pipeline(const char* vertex_source, std::ostream& diagnostics,
                            const char* fragment_source = kFragmentShaderSource) const {
        const GLuint vertex_shader = compile(GL_VERTEX_SHADER, vertex_source, diagnostics);
        if (vertex_shader == 0) {
            return 0;
        }

        const GLuint fragment_shader = compile(GL_FRAGMENT_SHADER, fragment_source, diagnostics);
        if (fragment_shader == 0) {
            glDeleteShader(vertex_shader);
            return 0;
        }

        const GLuint new_program = glCreateProgram();
        if (new_program == 0) {
            diagnostics << "render_error=program_create_failed\n";
            glDeleteShader(fragment_shader);
            glDeleteShader(vertex_shader);
            return 0;
        }

        glAttachShader(new_program, vertex_shader);
        glAttachShader(new_program, fragment_shader);
        glLinkProgram(new_program);
        glDeleteShader(fragment_shader);
        glDeleteShader(vertex_shader);

        GLint linked = GL_FALSE;
        glGetProgramiv(new_program, GL_LINK_STATUS, &linked);
        if (linked != GL_TRUE) {
            diagnostics << "render_error=program_link_failed\n";
            write_program_log(new_program, diagnostics);
            glDeleteProgram(new_program);
            return 0;
        }

        return new_program;
    }

    [[nodiscard]] bool create_pipelines(std::ostream& diagnostics) {
        triangle_program = create_program_pipeline(kTriangleVertexShaderSource, diagnostics);
        cube_program = create_program_pipeline(kVoxelCubeVertexShaderSource, diagnostics);
        paddock_program =
            create_program_pipeline(kHandcraftedPaddockVertexShaderSource, diagnostics,
                                    kHandcraftedPaddockFragmentShaderSource);
        paddock_shadow_program =
            create_program_pipeline(kHandcraftedPaddockShadowVertexShaderSource, diagnostics,
                                    kDepthOnlyFragmentShaderSource);
        paddock_debug_line_program =
            create_program_pipeline(kPaddockDebugLineVertexShaderSource, diagnostics);
        dog_program = create_program_pipeline(kDogVertexShaderSource, diagnostics);
        sheep_program = create_program_pipeline(kSheepVertexShaderSource, diagnostics,
                                                kSheepFragmentShaderSource);
        if (triangle_program == 0 || cube_program == 0 || paddock_program == 0 ||
            paddock_shadow_program == 0 || paddock_debug_line_program == 0 || dog_program == 0 ||
            sheep_program == 0) {
            return false;
        }

        cube_aspect_ratio = glGetUniformLocation(cube_program, "aspect_ratio");
        paddock_aspect_ratio = glGetUniformLocation(paddock_program, "aspect_ratio");
        paddock_camera_eye = glGetUniformLocation(paddock_program, "camera_eye");
        paddock_camera_target = glGetUniformLocation(paddock_program, "camera_target");
        paddock_palette = glGetUniformLocation(paddock_program, "material_palette[0]");
        paddock_shadow_map = glGetUniformLocation(paddock_program, "shadow_map");
        paddock_debug_mode = glGetUniformLocation(paddock_program, "debug_mode");
        paddock_debug_line_aspect_ratio =
            glGetUniformLocation(paddock_debug_line_program, "aspect_ratio");
        paddock_debug_line_camera_eye =
            glGetUniformLocation(paddock_debug_line_program, "camera_eye");
        paddock_debug_line_camera_target =
            glGetUniformLocation(paddock_debug_line_program, "camera_target");
        dog_aspect_ratio = glGetUniformLocation(dog_program, "aspect_ratio");
        dog_camera_eye = glGetUniformLocation(dog_program, "camera_eye");
        dog_camera_target = glGetUniformLocation(dog_program, "camera_target");
        dog_position = glGetUniformLocation(dog_program, "dog_position");
        dog_heading = glGetUniformLocation(dog_program, "dog_heading");
        sheep_aspect_ratio = glGetUniformLocation(sheep_program, "aspect_ratio");
        sheep_camera_eye = glGetUniformLocation(sheep_program, "camera_eye");
        sheep_camera_target = glGetUniformLocation(sheep_program, "camera_target");
        sheep_position = glGetUniformLocation(sheep_program, "sheep_position");
        sheep_heading = glGetUniformLocation(sheep_program, "sheep_heading");
        sheep_id = glGetUniformLocation(sheep_program, "sheep_id");
        sheep_shadow_map = glGetUniformLocation(sheep_program, "shadow_map");
        if (cube_aspect_ratio < 0 || paddock_aspect_ratio < 0 || paddock_palette < 0 ||
            paddock_camera_eye < 0 || paddock_camera_target < 0 || paddock_shadow_map < 0 ||
            paddock_debug_mode < 0 || paddock_debug_line_aspect_ratio < 0 ||
            paddock_debug_line_camera_eye < 0 || paddock_debug_line_camera_target < 0 ||
            dog_aspect_ratio < 0 || dog_camera_eye < 0 || dog_camera_target < 0 ||
            dog_position < 0 || dog_heading < 0 || sheep_aspect_ratio < 0 || sheep_camera_eye < 0 ||
            sheep_camera_target < 0 || sheep_position < 0 || sheep_heading < 0 || sheep_id < 0 ||
            sheep_shadow_map < 0) {
            diagnostics << "render_error=missing_required_uniform\n";
            return false;
        }

        glGenVertexArrays(1, &triangle_vertex_array);
        glGenBuffers(1, &triangle_vertex_buffer);
        glGenVertexArrays(1, &cube_vertex_array);
        glGenBuffers(1, &cube_vertex_buffer);
        glGenVertexArrays(1, &paddock_vertex_array);
        glGenBuffers(1, &paddock_vertex_buffer);
        glGenBuffers(1, &paddock_index_buffer);
        glGenVertexArrays(1, &paddock_debug_line_vertex_array);
        glGenBuffers(1, &paddock_debug_line_vertex_buffer);
        glGenVertexArrays(1, &influence_debug_vertex_array);
        glGenBuffers(1, &influence_debug_vertex_buffer);
        glGenVertexArrays(1, &dog_vertex_array);
        glGenBuffers(1, &dog_vertex_buffer);
        glGenVertexArrays(1, &sheep_vertex_array);
        glGenBuffers(1, &sheep_vertex_buffer);
        glGenFramebuffers(1, &paddock_shadow_framebuffer);
        glGenTextures(1, &paddock_shadow_texture);
        if (triangle_vertex_array == 0 || triangle_vertex_buffer == 0 || cube_vertex_array == 0 ||
            cube_vertex_buffer == 0 || paddock_vertex_array == 0 || paddock_vertex_buffer == 0 ||
            paddock_index_buffer == 0 || paddock_debug_line_vertex_array == 0 ||
            paddock_debug_line_vertex_buffer == 0 || influence_debug_vertex_array == 0 ||
            influence_debug_vertex_buffer == 0 || dog_vertex_array == 0 || dog_vertex_buffer == 0 ||
            sheep_vertex_array == 0 || sheep_vertex_buffer == 0 ||
            paddock_shadow_framebuffer == 0 || paddock_shadow_texture == 0) {
            diagnostics << "render_error=geometry_resource_create_failed\n";
            return false;
        }

        glBindVertexArray(triangle_vertex_array);
        glBindBuffer(GL_ARRAY_BUFFER, triangle_vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(kTriangleVertices)),
                     kTriangleVertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                              static_cast<GLsizei>(sizeof(TriangleVertex)), nullptr);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                              static_cast<GLsizei>(sizeof(TriangleVertex)),
                              reinterpret_cast<const void*>(offsetof(TriangleVertex, color)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(paddock_vertex_array);
        glBindBuffer(GL_ARRAY_BUFFER, paddock_vertex_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, paddock_index_buffer);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                              static_cast<GLsizei>(sizeof(voxel::ChunkMeshVertex)), nullptr);
        glEnableVertexAttribArray(0);
        glVertexAttribIPointer(
            1, 3, GL_BYTE, static_cast<GLsizei>(sizeof(voxel::ChunkMeshVertex)),
            reinterpret_cast<const void*>(offsetof(voxel::ChunkMeshVertex, normal)));
        glEnableVertexAttribArray(1);
        glVertexAttribIPointer(
            2, 1, GL_UNSIGNED_BYTE, static_cast<GLsizei>(sizeof(voxel::ChunkMeshVertex)),
            reinterpret_cast<const void*>(offsetof(voxel::ChunkMeshVertex, material)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(paddock_debug_line_vertex_array);
        glBindBuffer(GL_ARRAY_BUFFER, paddock_debug_line_vertex_buffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                              static_cast<GLsizei>(sizeof(DebugLineVertex)), nullptr);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                              static_cast<GLsizei>(sizeof(DebugLineVertex)),
                              reinterpret_cast<const void*>(offsetof(DebugLineVertex, color)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(influence_debug_vertex_array);
        glBindBuffer(GL_ARRAY_BUFFER, influence_debug_vertex_buffer);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(kMaximumInfluenceDebugSegments * 2U * sizeof(DebugLineVertex)),
            nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                              static_cast<GLsizei>(sizeof(DebugLineVertex)), nullptr);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                              static_cast<GLsizei>(sizeof(DebugLineVertex)),
                              reinterpret_cast<const void*>(offsetof(DebugLineVertex, color)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(cube_vertex_array);
        glBindBuffer(GL_ARRAY_BUFFER, cube_vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(kVoxelCubeVertices)),
                     kVoxelCubeVertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(CubeVertex)),
                              nullptr);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(CubeVertex)),
                              reinterpret_cast<const void*>(offsetof(CubeVertex, color)));
        glEnableVertexAttribArray(1);

        const std::vector<DogVertex> dog_vertices = make_placeholder_dog_vertices();
        if (dog_vertices.size() > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max())) {
            diagnostics << "render_error=dog_geometry_unrepresentable\n";
            return false;
        }
        dog_vertex_count = static_cast<GLsizei>(dog_vertices.size());
        glBindVertexArray(dog_vertex_array);
        glBindBuffer(GL_ARRAY_BUFFER, dog_vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(dog_vertices.size() * sizeof(DogVertex)),
                     dog_vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(DogVertex)),
                              nullptr);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(DogVertex)),
                              reinterpret_cast<const void*>(offsetof(DogVertex, color)));
        glEnableVertexAttribArray(1);

        const std::vector<SheepVertex> sheep_vertices = make_sheep_proxy_vertices();
        if (sheep_vertices.size() > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max())) {
            diagnostics << "render_error=sheep_geometry_unrepresentable\n";
            return false;
        }
        sheep_vertex_count = static_cast<GLsizei>(sheep_vertices.size());
        glBindVertexArray(sheep_vertex_array);
        glBindBuffer(GL_ARRAY_BUFFER, sheep_vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(sheep_vertices.size() * sizeof(SheepVertex)),
                     sheep_vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(SheepVertex)),
                              nullptr);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(SheepVertex)),
                              reinterpret_cast<const void*>(offsetof(SheepVertex, normal)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(SheepVertex)),
                              reinterpret_cast<const void*>(offsetof(SheepVertex, color)));
        glEnableVertexAttribArray(2);

        glUseProgram(sheep_program);
        glUniform1i(sheep_shadow_map, 0);
        glUseProgram(0);

        glBindTexture(GL_TEXTURE_2D, paddock_shadow_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, kShadowMapExtent, kShadowMapExtent, 0,
                     GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        constexpr std::array<float, 4> kShadowBorder{1.0F, 1.0F, 1.0F, 1.0F};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, kShadowBorder.data());

        glBindFramebuffer(GL_FRAMEBUFFER, paddock_shadow_framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                               paddock_shadow_texture, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            diagnostics << "render_error=shadow_framebuffer_incomplete\n";
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        return true;
    }
};

OpenGlRenderer::OpenGlRenderer() = default;

OpenGlRenderer::~OpenGlRenderer() = default;

bool OpenGlRenderer::initialize(std::ostream& diagnostics) {
    if (impl_ != nullptr) {
        diagnostics << "render_error=renderer_already_initialized\n";
        return false;
    }

    impl_ = std::make_unique<Impl>();
    if (!impl_->create_pipelines(diagnostics)) {
        return false;
    }

    return true;
}

bool OpenGlRenderer::upload_handcrafted_paddock(const voxel::ChunkMesh& mesh,
                                                const voxel::PaddockPalette& palette,
                                                std::size_t source_chunk_count,
                                                std::size_t occupied_block_count,
                                                std::ostream& diagnostics) {
    if (impl_ == nullptr) {
        diagnostics << "render_error=renderer_not_initialized\n";
        return false;
    }
    if (mesh.vertices.empty() || mesh.indices.empty()) {
        diagnostics << "render_error=paddock_mesh_empty\n";
        return false;
    }
    if (mesh.indices.size() > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()) ||
        mesh.vertices.size() >
            static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
        diagnostics << "render_error=paddock_mesh_count_unrepresentable\n";
        return false;
    }
    for (const std::uint32_t index : mesh.indices) {
        if (index >= mesh.vertices.size()) {
            diagnostics << "render_error=paddock_mesh_index_out_of_range\n";
            return false;
        }
    }

    const std::size_t maximum_upload_bytes =
        static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max());
    if (mesh.vertices.size() > maximum_upload_bytes / sizeof(voxel::ChunkMeshVertex) ||
        mesh.indices.size() > maximum_upload_bytes / sizeof(std::uint32_t)) {
        diagnostics << "render_error=paddock_mesh_bytes_unrepresentable\n";
        return false;
    }

    const auto vertex_bytes =
        static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(voxel::ChunkMeshVertex));
    const auto index_bytes = static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(std::uint32_t));
    glBindVertexArray(impl_->paddock_vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, impl_->paddock_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, vertex_bytes, mesh.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, impl_->paddock_index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_bytes, mesh.indices.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    std::vector<DebugLineVertex> debug_lines;
    debug_lines.reserve(4U * 24U + mesh.face_count() * 2U);
    impl_->paddock_chunk_bounds_first = 0;
    constexpr float kChunkEdge = static_cast<float>(voxel::Chunk::kEdgeLength);
    for (int chunk_z = 0; chunk_z < 2; ++chunk_z) {
        for (int chunk_x = 0; chunk_x < 2; ++chunk_x) {
            const float minimum_x = static_cast<float>(chunk_x) * kChunkEdge;
            const float minimum_z = static_cast<float>(chunk_z) * kChunkEdge;
            append_chunk_box(debug_lines, minimum_x, minimum_z, minimum_x + kChunkEdge,
                             minimum_z + kChunkEdge);
        }
    }
    impl_->paddock_chunk_bounds_count = static_cast<GLsizei>(debug_lines.size());
    impl_->paddock_face_normals_first = impl_->paddock_chunk_bounds_count;

    for (std::size_t face = 0; face < mesh.face_count(); ++face) {
        const std::size_t first_vertex = face * voxel::kNaiveMeshVerticesPerFace;
        std::array<float, 3> center{};
        for (std::size_t corner = 0; corner < voxel::kNaiveMeshVerticesPerFace; ++corner) {
            const auto& position = mesh.vertices[first_vertex + corner].position;
            center[0] += position[0] / static_cast<float>(voxel::kNaiveMeshVerticesPerFace);
            center[1] += position[1] / static_cast<float>(voxel::kNaiveMeshVerticesPerFace);
            center[2] += position[2] / static_cast<float>(voxel::kNaiveMeshVerticesPerFace);
        }
        const auto& source_normal = mesh.vertices[first_vertex].normal;
        const std::array<float, 3> normal{
            static_cast<float>(source_normal[0]),
            static_cast<float>(source_normal[1]),
            static_cast<float>(source_normal[2]),
        };
        const std::array<float, 3> start{
            center[0] + normal[0] * 0.025F,
            center[1] + normal[1] * 0.025F,
            center[2] + normal[2] * 0.025F,
        };
        const std::array<float, 3> end{
            center[0] + normal[0] * 0.42F,
            center[1] + normal[1] * 0.42F,
            center[2] + normal[2] * 0.42F,
        };
        append_debug_line(debug_lines, start, end, kFaceNormalColor);
    }
    const std::size_t face_normal_vertex_count =
        debug_lines.size() - static_cast<std::size_t>(impl_->paddock_face_normals_first);
    if (debug_lines.size() > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()) ||
        face_normal_vertex_count > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()) ||
        debug_lines.size() > maximum_upload_bytes / sizeof(DebugLineVertex)) {
        diagnostics << "render_error=paddock_debug_geometry_unrepresentable\n";
        return false;
    }
    impl_->paddock_face_normals_count = static_cast<GLsizei>(face_normal_vertex_count);
    glBindBuffer(GL_ARRAY_BUFFER, impl_->paddock_debug_line_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(debug_lines.size() * sizeof(DebugLineVertex)),
                 debug_lines.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    static_assert(sizeof(voxel::PaddockPaletteColor) == 3U * sizeof(float));
    glUseProgram(impl_->paddock_program);
    glUniform3fv(impl_->paddock_palette, static_cast<GLsizei>(palette.entries().size()),
                 palette.entries().front().rgb.data());
    glUniform1i(impl_->paddock_shadow_map, 0);
    glUniform1i(impl_->paddock_debug_mode, 0);
    glUseProgram(0);

    glBindFramebuffer(GL_FRAMEBUFFER, impl_->paddock_shadow_framebuffer);
    glViewport(0, 0, kShadowMapExtent, kShadowMapExtent);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0F, 4.0F);
    glClearDepth(1.0);
    glClear(GL_DEPTH_BUFFER_BIT);
    glUseProgram(impl_->paddock_shadow_program);
    glBindVertexArray(impl_->paddock_vertex_array);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indices.size()), GL_UNSIGNED_INT,
                   nullptr);
    glBindVertexArray(0);
    glUseProgram(0);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    const GLenum upload_error = glGetError();
    if (upload_error != GL_NO_ERROR) {
        diagnostics << "render_error=paddock_mesh_upload_failed gl_error=" << upload_error << '\n';
        return false;
    }

    impl_->paddock_index_count = static_cast<GLsizei>(mesh.indices.size());
    impl_->paddock_source_chunk_count = source_chunk_count;
    impl_->paddock_occupied_block_count = occupied_block_count;
    impl_->paddock_vertex_count = mesh.vertices.size();
    return true;
}

void OpenGlRenderer::render_triangle(int pixel_width, int pixel_height) const {
    glViewport(0, 0, pixel_width, pixel_height);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.035F, 0.055F, 0.09F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(impl_->triangle_program);
    glBindVertexArray(impl_->triangle_vertex_array);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glUseProgram(0);
}

void OpenGlRenderer::render_voxel_cube(int pixel_width, int pixel_height) const {
    glViewport(0, 0, pixel_width, pixel_height);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glClearDepth(1.0);
    glClearColor(0.035F, 0.055F, 0.09F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(impl_->cube_program);
    glUniform1f(impl_->cube_aspect_ratio,
                static_cast<float>(pixel_width) / static_cast<float>(pixel_height));
    glBindVertexArray(impl_->cube_vertex_array);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(kVoxelCubeVertices.size()));
    glBindVertexArray(0);
    glUseProgram(0);
}

void OpenGlRenderer::render_voxel_cube_wireframe(int pixel_width, int pixel_height) const {
    glViewport(0, 0, pixel_width, pixel_height);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glClearDepth(1.0);
    glClearColor(0.035F, 0.055F, 0.09F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(impl_->cube_program);
    glUniform1f(impl_->cube_aspect_ratio,
                static_cast<float>(pixel_width) / static_cast<float>(pixel_height));
    glBindVertexArray(impl_->cube_vertex_array);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(kVoxelCubeVertices.size()));
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindVertexArray(0);
    glUseProgram(0);
}

void OpenGlRenderer::render_handcrafted_paddock(int pixel_width, int pixel_height,
                                                HandcraftedPaddockFrame frame) const {
    const HandcraftedPaddockView view = frame.view;
    glViewport(0, 0, pixel_width, pixel_height);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glDisable(GL_BLEND);
    glClearDepth(1.0);
    glClearColor(kSkyColor[0], kSkyColor[1], kSkyColor[2], 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(impl_->paddock_program);
    glUniform1f(impl_->paddock_aspect_ratio,
                static_cast<float>(pixel_width) / static_cast<float>(pixel_height));
    glUniform3fv(impl_->paddock_camera_eye, 1, frame.camera.eye.data());
    glUniform3fv(impl_->paddock_camera_target, 1, frame.camera.target.data());
    glUniform1i(impl_->paddock_debug_mode,
                view == HandcraftedPaddockView::wireframe
                    ? 1
                    : (view == HandcraftedPaddockView::face_normals ? 2 : 0));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, impl_->paddock_shadow_texture);
    glBindVertexArray(impl_->paddock_vertex_array);
    if (view == HandcraftedPaddockView::wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    glDrawElements(GL_TRIANGLES, impl_->paddock_index_count, GL_UNSIGNED_INT, nullptr);
    if (view == HandcraftedPaddockView::wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

    const std::size_t sheep_count = std::min(frame.sheep_count, frame.sheep.size());
    if (sheep_count > 0) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
        glDisable(GL_CULL_FACE);
        glUseProgram(impl_->sheep_program);
        glUniform1f(impl_->sheep_aspect_ratio,
                    static_cast<float>(pixel_width) / static_cast<float>(pixel_height));
        glUniform3fv(impl_->sheep_camera_eye, 1, frame.camera.eye.data());
        glUniform3fv(impl_->sheep_camera_target, 1, frame.camera.target.data());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, impl_->paddock_shadow_texture);
        glBindVertexArray(impl_->sheep_vertex_array);
        for (std::size_t index = 0; index < sheep_count; ++index) {
            const SheepProxyPose& sheep = frame.sheep[index];
            glUniform3fv(impl_->sheep_position, 1, sheep.ground_position.data());
            glUniform1f(impl_->sheep_heading, sheep.heading_radians);
            glUniform1ui(impl_->sheep_id, sheep.id);
            glDrawArrays(GL_TRIANGLES, 0, impl_->sheep_vertex_count);
        }
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
        glEnable(GL_CULL_FACE);
    }

    if (frame.dog.has_value()) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
        glDisable(GL_CULL_FACE);
        glUseProgram(impl_->dog_program);
        glUniform1f(impl_->dog_aspect_ratio,
                    static_cast<float>(pixel_width) / static_cast<float>(pixel_height));
        glUniform3fv(impl_->dog_camera_eye, 1, frame.camera.eye.data());
        glUniform3fv(impl_->dog_camera_target, 1, frame.camera.target.data());
        glUniform3fv(impl_->dog_position, 1, frame.dog->ground_position.data());
        glUniform1f(impl_->dog_heading, frame.dog->heading_radians);
        glBindVertexArray(impl_->dog_vertex_array);
        glDrawArrays(GL_TRIANGLES, 0, impl_->dog_vertex_count);
        glBindVertexArray(0);
        glUseProgram(0);
        glEnable(GL_CULL_FACE);
    }

    if (view == HandcraftedPaddockView::chunk_bounds ||
        view == HandcraftedPaddockView::face_normals) {
        glDisable(GL_CULL_FACE);
        if (view == HandcraftedPaddockView::chunk_bounds) {
            glDisable(GL_DEPTH_TEST);
        } else {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_FALSE);
        }
        glUseProgram(impl_->paddock_debug_line_program);
        glUniform1f(impl_->paddock_debug_line_aspect_ratio,
                    static_cast<float>(pixel_width) / static_cast<float>(pixel_height));
        glUniform3fv(impl_->paddock_debug_line_camera_eye, 1, frame.camera.eye.data());
        glUniform3fv(impl_->paddock_debug_line_camera_target, 1, frame.camera.target.data());
        glBindVertexArray(impl_->paddock_debug_line_vertex_array);
        if (view == HandcraftedPaddockView::chunk_bounds) {
            glDrawArrays(GL_LINES, impl_->paddock_chunk_bounds_first,
                         impl_->paddock_chunk_bounds_count);
        } else {
            glDrawArrays(GL_LINES, impl_->paddock_face_normals_first,
                         impl_->paddock_face_normals_count);
        }
        glBindVertexArray(0);
        glUseProgram(0);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
    }

    if (view == HandcraftedPaddockView::mesh_statistics) {
        const int panel_width = std::min(pixel_width / 3, 260);
        const int panel_height = std::min(pixel_height / 3, 130);
        const int panel_x = 12;
        const int panel_y = pixel_height - panel_height - 12;
        if (panel_width > 32 && panel_height > 32 && panel_y >= 0) {
            glEnable(GL_SCISSOR_TEST);
            glScissor(panel_x, panel_y, panel_width, panel_height);
            glClearColor(0.025F, 0.035F, 0.055F, 1.0F);
            glClear(GL_COLOR_BUFFER_BIT);

            const std::array<std::size_t, 5> values{
                impl_->paddock_source_chunk_count,
                impl_->paddock_occupied_block_count,
                static_cast<std::size_t>(impl_->paddock_index_count) /
                    voxel::kNaiveMeshIndicesPerFace,
                impl_->paddock_vertex_count,
                static_cast<std::size_t>(impl_->paddock_index_count),
            };
            constexpr std::array<std::size_t, 5> scales{4U, 4096U, 4096U, 16384U, 24576U};
            constexpr std::array<std::array<float, 3>, 5> colors{{
                {{0.10F, 0.95F, 0.95F}},
                {{0.54F, 0.86F, 0.22F}},
                {{1.0F, 0.82F, 0.12F}},
                {{1.0F, 0.38F, 0.18F}},
                {{1.0F, 0.15F, 0.78F}},
            }};
            const int bar_x = panel_x + 12;
            const int maximum_bar_width = panel_width - 24;
            const int row_height = std::max((panel_height - 16) / 5, 1);
            for (std::size_t row = 0; row < values.size(); ++row) {
                const std::size_t clamped_value = std::min(values[row], scales[row]);
                const int bar_width = static_cast<int>(
                    clamped_value * static_cast<std::size_t>(maximum_bar_width) / scales[row]);
                glScissor(bar_x, panel_y + 8 + static_cast<int>(row) * row_height,
                          std::max(bar_width, 1), std::max(row_height - 4, 1));
                glClearColor(colors[row][0], colors[row][1], colors[row][2], 1.0F);
                glClear(GL_COLOR_BUFFER_BIT);
            }
            glDisable(GL_SCISSOR_TEST);
        }
    }
}

void OpenGlRenderer::render_influence_debug_overlay(int pixel_width, int pixel_height,
                                                    const CameraPose& camera,
                                                    const InfluenceDebugFrame& frame) const {
    if (impl_ == nullptr || frame.segment_count == 0) {
        return;
    }

    // Fixed scratch storage rather than a vector: the overlay is rebuilt every
    // frame, and a presentation path that heap-allocates once per frame is a
    // cost nobody asked for. The ceiling is the frame builder's own.
    std::array<DebugLineVertex, kMaximumInfluenceDebugSegments * 2U> vertices{};
    const std::size_t segment_count = std::min(frame.segment_count, frame.segments.size());
    for (std::size_t index = 0; index < segment_count; ++index) {
        const DebugSegment& segment = frame.segments[index];
        vertices[index * 2U] = {.position = segment.start, .color = segment.color};
        vertices[index * 2U + 1U] = {.position = segment.end, .color = segment.color};
    }
    const auto vertex_count = static_cast<GLsizei>(segment_count * 2U);

    glBindBuffer(GL_ARRAY_BUFFER, impl_->influence_debug_vertex_buffer);
    glBufferSubData(
        GL_ARRAY_BUFFER, 0,
        static_cast<GLsizeiptr>(static_cast<std::size_t>(vertex_count) * sizeof(DebugLineVertex)),
        vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Depth-tested with writes off, the same choice the face-normal overlay
    // makes: an arrow behind the barn stays behind the barn, so the diagram
    // reads as part of the scene rather than as a flat sheet over it.
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glUseProgram(impl_->paddock_debug_line_program);
    glUniform1f(impl_->paddock_debug_line_aspect_ratio,
                static_cast<float>(pixel_width) / static_cast<float>(pixel_height));
    glUniform3fv(impl_->paddock_debug_line_camera_eye, 1, camera.eye.data());
    glUniform3fv(impl_->paddock_debug_line_camera_target, 1, camera.target.data());
    glBindVertexArray(impl_->influence_debug_vertex_array);
    glDrawArrays(GL_LINES, 0, vertex_count);
    glBindVertexArray(0);
    glUseProgram(0);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
}

TriangleSample OpenGlRenderer::sample_triangle_center(int pixel_width, int pixel_height) const {
    std::array<GLubyte, 4> rgba{};
    glReadPixels(pixel_width / 2, pixel_height / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    return TriangleSample{.red = rgba[0], .green = rgba[1], .blue = rgba[2], .alpha = rgba[3]};
}

VoxelCubeSample OpenGlRenderer::sample_voxel_cube_center(int pixel_width, int pixel_height) const {
    std::array<GLubyte, 4> rgba{};
    GLfloat depth = 1.0F;
    GLboolean depth_write_enabled = GL_FALSE;
    GLint depth_function = 0;

    glReadPixels(pixel_width / 2, pixel_height / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glReadPixels(pixel_width / 2, pixel_height / 2, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depth_write_enabled);
    glGetIntegerv(GL_DEPTH_FUNC, &depth_function);

    return VoxelCubeSample{
        .color = {.red = rgba[0], .green = rgba[1], .blue = rgba[2], .alpha = rgba[3]},
        .depth = depth,
        .depth_test_enabled = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE,
        .depth_function_less = depth_function == GL_LESS,
        .depth_write_enabled = depth_write_enabled == GL_TRUE,
    };
}

VoxelCubeSample OpenGlRenderer::sample_handcrafted_paddock_center(int pixel_width,
                                                                  int pixel_height) const {
    return sample_voxel_cube_center(pixel_width, pixel_height);
}

std::optional<Rgba8Frame> OpenGlRenderer::capture_rgba8(int pixel_width, int pixel_height,
                                                        std::ostream& diagnostics) const {
    if (pixel_width <= 0 || pixel_height <= 0) {
        diagnostics << "capture_error=invalid_dimensions width=" << pixel_width
                    << " height=" << pixel_height << '\n';
        return std::nullopt;
    }

    constexpr std::size_t kBytesPerPixel = 4;
    const auto width = static_cast<std::uint32_t>(pixel_width);
    const auto height = static_cast<std::uint32_t>(pixel_height);
    const std::uint64_t pixel_bytes = static_cast<std::uint64_t>(width) * height * kBytesPerPixel;
    if (pixel_bytes > std::numeric_limits<std::size_t>::max()) {
        diagnostics << "capture_error=image_size_overflow\n";
        return std::nullopt;
    }

    Rgba8Frame frame{
        .width = width,
        .height = height,
        .pixels = std::vector<std::uint8_t>(static_cast<std::size_t>(pixel_bytes)),
    };
    glReadPixels(0, 0, pixel_width, pixel_height, GL_RGBA, GL_UNSIGNED_BYTE, frame.pixels.data());

    const std::size_t row_bytes = static_cast<std::size_t>(width) * kBytesPerPixel;
    for (std::uint32_t row = 0; row < height / 2U; ++row) {
        const std::size_t top_begin = static_cast<std::size_t>(row) * row_bytes;
        const std::size_t bottom_begin = static_cast<std::size_t>(height - row - 1U) * row_bytes;
        std::swap_ranges(frame.pixels.begin() + static_cast<std::ptrdiff_t>(top_begin),
                         frame.pixels.begin() + static_cast<std::ptrdiff_t>(top_begin + row_bytes),
                         frame.pixels.begin() + static_cast<std::ptrdiff_t>(bottom_begin));
    }
    return frame;
}

bool is_expected_triangle_sample(const TriangleSample& sample) {
    constexpr std::uint8_t kMinimumTriangleChannel = 64;
    return sample.red > kMinimumTriangleChannel && sample.green > kMinimumTriangleChannel &&
           sample.blue > kMinimumTriangleChannel;
}

bool is_expected_voxel_cube_sample(const VoxelCubeSample& sample) {
    constexpr std::uint8_t kMinimumFrontRed = 180;
    constexpr std::uint8_t kMaximumFrontGreen = 100;
    constexpr std::uint8_t kMaximumFrontBlue = 80;
    constexpr std::uint8_t kMinimumAlpha = 250;

    const bool front_face_visible =
        sample.color.red > kMinimumFrontRed && sample.color.green < kMaximumFrontGreen &&
        sample.color.blue < kMaximumFrontBlue && sample.color.alpha >= kMinimumAlpha;
    const bool fragment_depth_written = sample.depth > 0.0F && sample.depth < 1.0F;
    return front_face_visible && fragment_depth_written && sample.depth_test_enabled &&
           sample.depth_function_less && sample.depth_write_enabled;
}

bool is_expected_handcrafted_paddock_sample(const VoxelCubeSample& sample) {
    constexpr std::uint8_t kMinimumGateRed = 80;
    constexpr std::uint8_t kMaximumGateGreen = 95;
    constexpr std::uint8_t kMaximumGateBlue = 100;
    constexpr std::uint8_t kMinimumAlpha = 250;

    const bool red_gate_visible =
        sample.color.red > kMinimumGateRed && sample.color.green < kMaximumGateGreen &&
        sample.color.blue < kMaximumGateBlue &&
        sample.color.red > static_cast<unsigned int>(sample.color.green) * 3U / 2U &&
        sample.color.red > static_cast<unsigned int>(sample.color.blue) * 3U / 2U &&
        sample.color.alpha >= kMinimumAlpha;
    const bool fragment_depth_written = sample.depth > 0.0F && sample.depth < 1.0F;
    return red_gate_visible && fragment_depth_written && sample.depth_test_enabled &&
           sample.depth_function_less && sample.depth_write_enabled;
}

std::size_t count_handcrafted_paddock_debug_pixels(const Rgba8Frame& frame,
                                                   HandcraftedPaddockView view) {
    constexpr std::size_t kBytesPerPixel = 4;
    std::size_t matching_pixels = 0;
    for (std::size_t index = 0; index + 3U < frame.pixels.size(); index += kBytesPerPixel) {
        const std::uint8_t red = frame.pixels[index];
        const std::uint8_t green = frame.pixels[index + 1U];
        const std::uint8_t blue = frame.pixels[index + 2U];
        bool matches = false;
        switch (view) {
        case HandcraftedPaddockView::chunk_bounds:
            matches = red < 80U && green > 200U && blue > 200U;
            break;
        case HandcraftedPaddockView::face_normals:
            matches = red > 210U && green < 100U && blue > 160U;
            break;
        case HandcraftedPaddockView::wireframe:
            matches = red > 210U && green > 160U && blue < 90U;
            break;
        case HandcraftedPaddockView::mesh_statistics:
            matches = red > 210U && green < 100U && blue > 160U;
            break;
        case HandcraftedPaddockView::normal:
            break;
        }
        if (matches) {
            ++matching_pixels;
        }
    }
    return matching_pixels;
}

bool is_expected_handcrafted_paddock_debug_frame(const Rgba8Frame& frame,
                                                 HandcraftedPaddockView view) {
    constexpr std::size_t kBytesPerPixel = 4;
    const std::uint64_t expected_bytes =
        static_cast<std::uint64_t>(frame.width) * frame.height * kBytesPerPixel;
    if (view == HandcraftedPaddockView::normal || frame.width == 0U || frame.height == 0U ||
        expected_bytes != frame.pixels.size()) {
        return false;
    }

    constexpr std::size_t kMinimumDebugPixels = 24;
    const std::size_t matching_pixels = count_handcrafted_paddock_debug_pixels(frame, view);
    const std::size_t total_pixels = static_cast<std::size_t>(frame.width) * frame.height;
    return matching_pixels >= kMinimumDebugPixels && matching_pixels < total_pixels / 2U;
}

std::size_t count_voxel_cube_wireframe_pixels(const Rgba8Frame& frame) {
    constexpr std::uint8_t kMinimumVisibleChannel = 40;
    constexpr std::size_t kBytesPerPixel = 4;
    std::size_t visible_pixels = 0;
    for (std::size_t index = 0; index + 3U < frame.pixels.size(); index += kBytesPerPixel) {
        if (frame.pixels[index] > kMinimumVisibleChannel ||
            frame.pixels[index + 1U] > kMinimumVisibleChannel ||
            frame.pixels[index + 2U] > kMinimumVisibleChannel) {
            ++visible_pixels;
        }
    }
    return visible_pixels;
}

bool is_expected_voxel_cube_wireframe(const Rgba8Frame& frame) {
    constexpr std::size_t kMinimumVisiblePixels = 16;
    constexpr std::size_t kBytesPerPixel = 4;
    const std::uint64_t expected_bytes =
        static_cast<std::uint64_t>(frame.width) * frame.height * kBytesPerPixel;
    if (frame.width == 0U || frame.height == 0U || expected_bytes != frame.pixels.size()) {
        return false;
    }

    const std::size_t visible_pixels = count_voxel_cube_wireframe_pixels(frame);
    const std::size_t total_pixels = static_cast<std::size_t>(frame.width) * frame.height;
    return visible_pixels >= kMinimumVisiblePixels && visible_pixels < total_pixels / 2U;
}

std::array<std::size_t, kInfluenceChannelCount>
count_influence_debug_channel_pixels(const Rgba8Frame& frame) {
    // The overlay is unlit, so a drawn fragment carries its vertex colour
    // exactly. A narrow band around each entry absorbs the float-to-8-bit
    // rounding without letting two lanes collide.
    constexpr std::size_t kBytesPerPixel = 4;
    constexpr int kChannelTolerance = 6;
    std::array<std::array<int, 3>, kInfluenceChannelCount> expected{};
    for (std::size_t lane = 0; lane < kInfluenceChannelCount; ++lane) {
        const std::array<float, 3>& color =
            influence_channel_color(static_cast<InfluenceChannel>(lane));
        for (std::size_t component = 0; component < 3U; ++component) {
            expected[lane][component] = static_cast<int>(std::lround(color[component] * 255.0F));
        }
    }

    std::array<std::size_t, kInfluenceChannelCount> counts{};
    for (std::size_t index = 0; index + 3U < frame.pixels.size(); index += kBytesPerPixel) {
        const std::array<int, 3> pixel{static_cast<int>(frame.pixels[index]),
                                       static_cast<int>(frame.pixels[index + 1U]),
                                       static_cast<int>(frame.pixels[index + 2U])};
        for (std::size_t lane = 0; lane < kInfluenceChannelCount; ++lane) {
            if (std::abs(pixel[0] - expected[lane][0]) <= kChannelTolerance &&
                std::abs(pixel[1] - expected[lane][1]) <= kChannelTolerance &&
                std::abs(pixel[2] - expected[lane][2]) <= kChannelTolerance) {
                ++counts[lane];
                break;
            }
        }
    }
    return counts;
}

bool is_expected_influence_debug_frame(const Rgba8Frame& frame) {
    constexpr std::size_t kBytesPerPixel = 4;
    const std::uint64_t expected_bytes =
        static_cast<std::uint64_t>(frame.width) * frame.height * kBytesPerPixel;
    if (frame.width == 0U || frame.height == 0U || expected_bytes != frame.pixels.size()) {
        return false;
    }

    // The three social lanes always publish a tick, so three visible lanes is
    // what any scenario must reach; requiring more would make the oracle a
    // statement about which terms a particular scenario switches on.
    constexpr std::size_t kMinimumVisibleLanes = 3;
    constexpr std::size_t kMinimumLanePixels = 8;
    constexpr std::size_t kMinimumOverlayPixels = 64;
    const std::array<std::size_t, kInfluenceChannelCount> counts =
        count_influence_debug_channel_pixels(frame);
    std::size_t visible_lanes = 0;
    std::size_t overlay_pixels = 0;
    for (const std::size_t count : counts) {
        overlay_pixels += count;
        if (count >= kMinimumLanePixels) {
            ++visible_lanes;
        }
    }
    const std::size_t total_pixels = static_cast<std::size_t>(frame.width) * frame.height;
    return visible_lanes >= kMinimumVisibleLanes && overlay_pixels >= kMinimumOverlayPixels &&
           overlay_pixels < total_pixels / 2U;
}

} // namespace wide_eye::render
