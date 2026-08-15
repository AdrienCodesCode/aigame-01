#include "render/triangle_renderer.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <ostream>

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

using GlViewport = void(APIENTRY*)(GLint, GLint, GLsizei, GLsizei);
using GlClearColor = void(APIENTRY*)(GLfloat, GLfloat, GLfloat, GLfloat);
using GlClear = void(APIENTRY*)(GLbitfield);
using GlDrawArrays = void(APIENTRY*)(GLenum, GLint, GLsizei);
using GlReadPixels = void(APIENTRY*)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid*);
using GlEnable = void(APIENTRY*)(GLenum);
using GlDisable = void(APIENTRY*)(GLenum);
using GlDepthFunction = void(APIENTRY*)(GLenum);
using GlDepthMask = void(APIENTRY*)(GLboolean);
using GlClearDepth = void(APIENTRY*)(GLdouble);
using GlGetBoolean = void(APIENTRY*)(GLenum, GLboolean*);
using GlGetInteger = void(APIENTRY*)(GLenum, GLint*);
using GlIsEnabled = GLboolean(APIENTRY*)(GLenum);

template <typename Function>
bool load_gl_function(Function& function, const char* name, std::ostream& diagnostics) {
    function = reinterpret_cast<Function>(SDL_GL_GetProcAddress(name));
    if (function != nullptr) {
        return true;
    }

    diagnostics << "render_error=missing_gl_entry_point name=" << name << '\n';
    return false;
}

void write_shader_log(PFNGLGETSHADERINFOLOGPROC get_shader_info_log, GLuint shader,
                      std::ostream& diagnostics) {
    std::array<GLchar, 4096> log{};
    GLsizei written = 0;
    get_shader_info_log(shader, static_cast<GLsizei>(log.size()), &written, log.data());
    diagnostics << "shader_log=";
    if (written > 0) {
        diagnostics.write(log.data(), static_cast<std::streamsize>(written));
    } else {
        diagnostics << "<unavailable>";
    }
    diagnostics << '\n';
}

void write_program_log(PFNGLGETPROGRAMINFOLOGPROC get_program_info_log, GLuint program,
                       std::ostream& diagnostics) {
    std::array<GLchar, 4096> log{};
    GLsizei written = 0;
    get_program_info_log(program, static_cast<GLsizei>(log.size()), &written, log.data());
    diagnostics << "program_log=";
    if (written > 0) {
        diagnostics.write(log.data(), static_cast<std::streamsize>(written));
    } else {
        diagnostics << "<unavailable>";
    }
    diagnostics << '\n';
}

} // namespace

struct TriangleRenderer::Impl {
    PFNGLCREATESHADERPROC create_shader = nullptr;
    PFNGLSHADERSOURCEPROC shader_source = nullptr;
    PFNGLCOMPILESHADERPROC compile_shader = nullptr;
    PFNGLGETSHADERIVPROC get_shader_integer = nullptr;
    PFNGLGETSHADERINFOLOGPROC get_shader_info_log = nullptr;
    PFNGLDELETESHADERPROC delete_shader = nullptr;
    PFNGLCREATEPROGRAMPROC create_program = nullptr;
    PFNGLATTACHSHADERPROC attach_shader = nullptr;
    PFNGLLINKPROGRAMPROC link_program = nullptr;
    PFNGLGETPROGRAMIVPROC get_program_integer = nullptr;
    PFNGLGETPROGRAMINFOLOGPROC get_program_info_log = nullptr;
    PFNGLDELETEPROGRAMPROC delete_program = nullptr;
    PFNGLGENVERTEXARRAYSPROC generate_vertex_arrays = nullptr;
    PFNGLBINDVERTEXARRAYPROC bind_vertex_array = nullptr;
    PFNGLDELETEVERTEXARRAYSPROC delete_vertex_arrays = nullptr;
    PFNGLGENBUFFERSPROC generate_buffers = nullptr;
    PFNGLBINDBUFFERPROC bind_buffer = nullptr;
    PFNGLBUFFERDATAPROC buffer_data = nullptr;
    PFNGLDELETEBUFFERSPROC delete_buffers = nullptr;
    PFNGLVERTEXATTRIBPOINTERPROC vertex_attribute_pointer = nullptr;
    PFNGLENABLEVERTEXATTRIBARRAYPROC enable_vertex_attribute_array = nullptr;
    GlViewport viewport = nullptr;
    GlClearColor clear_color = nullptr;
    GlClear clear = nullptr;
    PFNGLUSEPROGRAMPROC use_program = nullptr;
    GlDrawArrays draw_arrays = nullptr;
    GlReadPixels read_pixels = nullptr;
    GlEnable enable = nullptr;
    GlDisable disable = nullptr;
    GlDepthFunction depth_function = nullptr;
    GlDepthMask depth_mask = nullptr;
    GlClearDepth clear_depth = nullptr;
    GlGetBoolean get_boolean = nullptr;
    GlGetInteger get_integer = nullptr;
    GlIsEnabled is_enabled = nullptr;
    PFNGLGETUNIFORMLOCATIONPROC get_uniform_location = nullptr;
    PFNGLUNIFORM1FPROC uniform_1f = nullptr;

    GLuint triangle_program = 0;
    GLuint triangle_vertex_array = 0;
    GLuint triangle_vertex_buffer = 0;
    GLuint cube_program = 0;
    GLuint cube_vertex_array = 0;
    GLuint cube_vertex_buffer = 0;
    GLint cube_aspect_ratio = -1;

    ~Impl() {
        if (cube_vertex_buffer != 0 && delete_buffers != nullptr) {
            delete_buffers(1, &cube_vertex_buffer);
        }
        if (triangle_vertex_buffer != 0 && delete_buffers != nullptr) {
            delete_buffers(1, &triangle_vertex_buffer);
        }
        if (cube_vertex_array != 0 && delete_vertex_arrays != nullptr) {
            delete_vertex_arrays(1, &cube_vertex_array);
        }
        if (triangle_vertex_array != 0 && delete_vertex_arrays != nullptr) {
            delete_vertex_arrays(1, &triangle_vertex_array);
        }
        if (cube_program != 0 && delete_program != nullptr) {
            delete_program(cube_program);
        }
        if (triangle_program != 0 && delete_program != nullptr) {
            delete_program(triangle_program);
        }
    }

    [[nodiscard]] bool load_functions(std::ostream& diagnostics) {
        return load_gl_function(create_shader, "glCreateShader", diagnostics) &&
               load_gl_function(shader_source, "glShaderSource", diagnostics) &&
               load_gl_function(compile_shader, "glCompileShader", diagnostics) &&
               load_gl_function(get_shader_integer, "glGetShaderiv", diagnostics) &&
               load_gl_function(get_shader_info_log, "glGetShaderInfoLog", diagnostics) &&
               load_gl_function(delete_shader, "glDeleteShader", diagnostics) &&
               load_gl_function(create_program, "glCreateProgram", diagnostics) &&
               load_gl_function(attach_shader, "glAttachShader", diagnostics) &&
               load_gl_function(link_program, "glLinkProgram", diagnostics) &&
               load_gl_function(get_program_integer, "glGetProgramiv", diagnostics) &&
               load_gl_function(get_program_info_log, "glGetProgramInfoLog", diagnostics) &&
               load_gl_function(delete_program, "glDeleteProgram", diagnostics) &&
               load_gl_function(generate_vertex_arrays, "glGenVertexArrays", diagnostics) &&
               load_gl_function(bind_vertex_array, "glBindVertexArray", diagnostics) &&
               load_gl_function(delete_vertex_arrays, "glDeleteVertexArrays", diagnostics) &&
               load_gl_function(generate_buffers, "glGenBuffers", diagnostics) &&
               load_gl_function(bind_buffer, "glBindBuffer", diagnostics) &&
               load_gl_function(buffer_data, "glBufferData", diagnostics) &&
               load_gl_function(delete_buffers, "glDeleteBuffers", diagnostics) &&
               load_gl_function(vertex_attribute_pointer, "glVertexAttribPointer", diagnostics) &&
               load_gl_function(enable_vertex_attribute_array, "glEnableVertexAttribArray",
                                diagnostics) &&
               load_gl_function(viewport, "glViewport", diagnostics) &&
               load_gl_function(clear_color, "glClearColor", diagnostics) &&
               load_gl_function(clear, "glClear", diagnostics) &&
               load_gl_function(use_program, "glUseProgram", diagnostics) &&
               load_gl_function(draw_arrays, "glDrawArrays", diagnostics) &&
               load_gl_function(read_pixels, "glReadPixels", diagnostics) &&
               load_gl_function(enable, "glEnable", diagnostics) &&
               load_gl_function(disable, "glDisable", diagnostics) &&
               load_gl_function(depth_function, "glDepthFunc", diagnostics) &&
               load_gl_function(depth_mask, "glDepthMask", diagnostics) &&
               load_gl_function(clear_depth, "glClearDepth", diagnostics) &&
               load_gl_function(get_boolean, "glGetBooleanv", diagnostics) &&
               load_gl_function(get_integer, "glGetIntegerv", diagnostics) &&
               load_gl_function(is_enabled, "glIsEnabled", diagnostics) &&
               load_gl_function(get_uniform_location, "glGetUniformLocation", diagnostics) &&
               load_gl_function(uniform_1f, "glUniform1f", diagnostics);
    }

    [[nodiscard]] GLuint compile(GLenum type, const char* source, std::ostream& diagnostics) const {
        const GLuint shader = create_shader(type);
        if (shader == 0) {
            diagnostics << "render_error=shader_create_failed\n";
            return 0;
        }

        shader_source(shader, 1, &source, nullptr);
        compile_shader(shader);

        GLint compiled = GL_FALSE;
        get_shader_integer(shader, GL_COMPILE_STATUS, &compiled);
        if (compiled == GL_TRUE) {
            return shader;
        }

        diagnostics << "render_error=shader_compile_failed type=" << type << '\n';
        write_shader_log(get_shader_info_log, shader, diagnostics);
        delete_shader(shader);
        return 0;
    }

    [[nodiscard]] GLuint create_program_pipeline(const char* vertex_source,
                                                 std::ostream& diagnostics) const {
        const GLuint vertex_shader = compile(GL_VERTEX_SHADER, vertex_source, diagnostics);
        if (vertex_shader == 0) {
            return 0;
        }

        const GLuint fragment_shader =
            compile(GL_FRAGMENT_SHADER, kFragmentShaderSource, diagnostics);
        if (fragment_shader == 0) {
            delete_shader(vertex_shader);
            return 0;
        }

        const GLuint new_program = create_program();
        if (new_program == 0) {
            diagnostics << "render_error=program_create_failed\n";
            delete_shader(fragment_shader);
            delete_shader(vertex_shader);
            return 0;
        }

        attach_shader(new_program, vertex_shader);
        attach_shader(new_program, fragment_shader);
        link_program(new_program);
        delete_shader(fragment_shader);
        delete_shader(vertex_shader);

        GLint linked = GL_FALSE;
        get_program_integer(new_program, GL_LINK_STATUS, &linked);
        if (linked != GL_TRUE) {
            diagnostics << "render_error=program_link_failed\n";
            write_program_log(get_program_info_log, new_program, diagnostics);
            delete_program(new_program);
            return 0;
        }

        return new_program;
    }

    [[nodiscard]] bool create_pipelines(std::ostream& diagnostics) {
        triangle_program = create_program_pipeline(kTriangleVertexShaderSource, diagnostics);
        cube_program = create_program_pipeline(kVoxelCubeVertexShaderSource, diagnostics);
        if (triangle_program == 0 || cube_program == 0) {
            return false;
        }

        cube_aspect_ratio = get_uniform_location(cube_program, "aspect_ratio");
        if (cube_aspect_ratio < 0) {
            diagnostics << "render_error=missing_cube_aspect_uniform\n";
            return false;
        }

        generate_vertex_arrays(1, &triangle_vertex_array);
        generate_buffers(1, &triangle_vertex_buffer);
        generate_vertex_arrays(1, &cube_vertex_array);
        generate_buffers(1, &cube_vertex_buffer);
        if (triangle_vertex_array == 0 || triangle_vertex_buffer == 0 || cube_vertex_array == 0 ||
            cube_vertex_buffer == 0) {
            diagnostics << "render_error=geometry_resource_create_failed\n";
            return false;
        }

        bind_vertex_array(triangle_vertex_array);
        bind_buffer(GL_ARRAY_BUFFER, triangle_vertex_buffer);
        buffer_data(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(kTriangleVertices)),
                    kTriangleVertices.data(), GL_STATIC_DRAW);

        vertex_attribute_pointer(0, 2, GL_FLOAT, GL_FALSE,
                                 static_cast<GLsizei>(sizeof(TriangleVertex)), nullptr);
        enable_vertex_attribute_array(0);
        vertex_attribute_pointer(1, 3, GL_FLOAT, GL_FALSE,
                                 static_cast<GLsizei>(sizeof(TriangleVertex)),
                                 reinterpret_cast<const void*>(offsetof(TriangleVertex, color)));
        enable_vertex_attribute_array(1);

        bind_vertex_array(cube_vertex_array);
        bind_buffer(GL_ARRAY_BUFFER, cube_vertex_buffer);
        buffer_data(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(kVoxelCubeVertices)),
                    kVoxelCubeVertices.data(), GL_STATIC_DRAW);

        vertex_attribute_pointer(0, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(CubeVertex)),
                                 nullptr);
        enable_vertex_attribute_array(0);
        vertex_attribute_pointer(1, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(CubeVertex)),
                                 reinterpret_cast<const void*>(offsetof(CubeVertex, color)));
        enable_vertex_attribute_array(1);

        bind_buffer(GL_ARRAY_BUFFER, 0);
        bind_vertex_array(0);
        return true;
    }
};

TriangleRenderer::TriangleRenderer() = default;

TriangleRenderer::~TriangleRenderer() = default;

bool TriangleRenderer::initialize(std::ostream& diagnostics) {
    if (impl_ != nullptr) {
        diagnostics << "render_error=renderer_already_initialized\n";
        return false;
    }

    impl_ = std::make_unique<Impl>();
    if (!impl_->load_functions(diagnostics) || !impl_->create_pipelines(diagnostics)) {
        return false;
    }

    return true;
}

void TriangleRenderer::render(int pixel_width, int pixel_height) const {
    impl_->viewport(0, 0, pixel_width, pixel_height);
    impl_->disable(GL_DEPTH_TEST);
    impl_->clear_color(0.035F, 0.055F, 0.09F, 1.0F);
    impl_->clear(GL_COLOR_BUFFER_BIT);
    impl_->use_program(impl_->triangle_program);
    impl_->bind_vertex_array(impl_->triangle_vertex_array);
    impl_->draw_arrays(GL_TRIANGLES, 0, 3);
    impl_->bind_vertex_array(0);
    impl_->use_program(0);
}

void TriangleRenderer::render_voxel_cube(int pixel_width, int pixel_height) const {
    impl_->viewport(0, 0, pixel_width, pixel_height);
    impl_->enable(GL_DEPTH_TEST);
    impl_->depth_function(GL_LESS);
    impl_->depth_mask(GL_TRUE);
    impl_->clear_depth(1.0);
    impl_->clear_color(0.035F, 0.055F, 0.09F, 1.0F);
    impl_->clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    impl_->use_program(impl_->cube_program);
    impl_->uniform_1f(impl_->cube_aspect_ratio,
                      static_cast<float>(pixel_width) / static_cast<float>(pixel_height));
    impl_->bind_vertex_array(impl_->cube_vertex_array);
    impl_->draw_arrays(GL_TRIANGLES, 0, static_cast<GLsizei>(kVoxelCubeVertices.size()));
    impl_->bind_vertex_array(0);
    impl_->use_program(0);
}

TriangleSample TriangleRenderer::sample_center(int pixel_width, int pixel_height) const {
    std::array<GLubyte, 4> rgba{};
    impl_->read_pixels(pixel_width / 2, pixel_height / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                       rgba.data());
    return TriangleSample{.red = rgba[0], .green = rgba[1], .blue = rgba[2], .alpha = rgba[3]};
}

VoxelCubeSample TriangleRenderer::sample_voxel_cube_center(int pixel_width,
                                                           int pixel_height) const {
    std::array<GLubyte, 4> rgba{};
    GLfloat depth = 1.0F;
    GLboolean depth_write_enabled = GL_FALSE;
    GLint depth_function = 0;

    impl_->read_pixels(pixel_width / 2, pixel_height / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                       rgba.data());
    impl_->read_pixels(pixel_width / 2, pixel_height / 2, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT,
                       &depth);
    impl_->get_boolean(GL_DEPTH_WRITEMASK, &depth_write_enabled);
    impl_->get_integer(GL_DEPTH_FUNC, &depth_function);

    return VoxelCubeSample{
        .color = {.red = rgba[0], .green = rgba[1], .blue = rgba[2], .alpha = rgba[3]},
        .depth = depth,
        .depth_test_enabled = impl_->is_enabled(GL_DEPTH_TEST) == GL_TRUE,
        .depth_function_less = depth_function == GL_LESS,
        .depth_write_enabled = depth_write_enabled == GL_TRUE,
    };
}

std::optional<Rgba8Frame> TriangleRenderer::capture_rgba8(int pixel_width, int pixel_height,
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
    impl_->read_pixels(0, 0, pixel_width, pixel_height, GL_RGBA, GL_UNSIGNED_BYTE,
                       frame.pixels.data());

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

} // namespace wide_eye::render
