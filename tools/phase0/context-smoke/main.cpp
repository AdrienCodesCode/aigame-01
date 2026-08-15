#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

constexpr int kDefaultMajor = 4;
constexpr int kDefaultMinor = 6;

using GlGetString = const GLubyte*(APIENTRY*)(GLenum);
using GlGetInteger = void(APIENTRY*)(GLenum, GLint*);

int fail(std::string_view stage) {
  std::cerr << "result=fail\n"
            << "failure_stage=" << stage << '\n'
            << "sdl_error=" << SDL_GetError() << '\n';
  return EXIT_FAILURE;
}

bool parse_component(const char* text, int& value) {
  errno = 0;
  char* end = nullptr;
  const long parsed = std::strtol(text, &end, 10);
  if (errno != 0 || end == text || *end != '\0' || parsed < 0 ||
      parsed > std::numeric_limits<int>::max()) {
    return false;
  }

  value = static_cast<int>(parsed);
  return true;
}

const char* gl_string(GlGetString get_string, GLenum name) {
  const GLubyte* value = get_string(name);
  return value == nullptr ? "<unavailable>"
                          : reinterpret_cast<const char*>(value);
}

}  // namespace

int main(int argc, char** argv) {
  int requested_major = kDefaultMajor;
  int requested_minor = kDefaultMinor;

  if (argc != 1 && argc != 3) {
    std::cerr << "usage: " << argv[0] << " [major minor]\n";
    return EXIT_FAILURE;
  }
  if (argc == 3 &&
      (!parse_component(argv[1], requested_major) ||
       !parse_component(argv[2], requested_minor))) {
    std::cerr << "invalid OpenGL version components\n";
    return EXIT_FAILURE;
  }

  std::cout << "requested_gl=" << requested_major << '.' << requested_minor
            << '\n'
            << std::flush;

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    return fail("sdl_init");
  }

  const auto quit_sdl = []() { SDL_Quit(); };
  if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, requested_major) ||
      !SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, requested_minor) ||
      !SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                           SDL_GL_CONTEXT_PROFILE_CORE) ||
      !SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,
                           SDL_GL_CONTEXT_DEBUG_FLAG) ||
      !SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) ||
      !SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24)) {
    const int status = fail("gl_attributes");
    quit_sdl();
    return status;
  }

  SDL_Window* window = SDL_CreateWindow(
      "Wide Eye Phase 0 context smoke", 64, 64,
      SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  if (window == nullptr) {
    const int status = fail("window_create");
    quit_sdl();
    return status;
  }

  SDL_GLContext context = SDL_GL_CreateContext(window);
  if (context == nullptr) {
    const int status = fail("context_create");
    SDL_DestroyWindow(window);
    quit_sdl();
    return status;
  }

  if (!SDL_GL_MakeCurrent(window, context)) {
    const int status = fail("make_current");
    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    quit_sdl();
    return status;
  }

  const auto get_string = reinterpret_cast<GlGetString>(
      SDL_GL_GetProcAddress("glGetString"));
  const auto get_integer = reinterpret_cast<GlGetInteger>(
      SDL_GL_GetProcAddress("glGetIntegerv"));
  if (get_string == nullptr || get_integer == nullptr) {
    const int status = fail("gl_entry_points");
    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    quit_sdl();
    return status;
  }

  GLint actual_major = 0;
  GLint actual_minor = 0;
  GLint profile_mask = 0;
  GLint context_flags = 0;
  get_integer(GL_MAJOR_VERSION, &actual_major);
  get_integer(GL_MINOR_VERSION, &actual_minor);
  get_integer(GL_CONTEXT_PROFILE_MASK, &profile_mask);
  get_integer(GL_CONTEXT_FLAGS, &context_flags);

  const int sdl_version = SDL_GetVersion();
  std::cout << "sdl_version=" << SDL_VERSIONNUM_MAJOR(sdl_version) << '.'
            << SDL_VERSIONNUM_MINOR(sdl_version) << '.'
            << SDL_VERSIONNUM_MICRO(sdl_version) << '\n'
            << "sdl_revision=" << SDL_GetRevision() << '\n'
            << "video_driver="
            << (SDL_GetCurrentVideoDriver() == nullptr
                    ? "<unavailable>"
                    : SDL_GetCurrentVideoDriver())
            << '\n'
            << "gl_vendor=" << gl_string(get_string, GL_VENDOR) << '\n'
            << "gl_renderer=" << gl_string(get_string, GL_RENDERER) << '\n'
            << "gl_version=" << gl_string(get_string, GL_VERSION) << '\n'
            << "glsl_version="
            << gl_string(get_string, GL_SHADING_LANGUAGE_VERSION) << '\n'
            << "actual_gl=" << actual_major << '.' << actual_minor << '\n'
            << "core_profile="
            << ((profile_mask & GL_CONTEXT_CORE_PROFILE_BIT) != 0 ? "yes"
                                                                  : "no")
            << '\n'
            << "debug_context="
            << ((context_flags & GL_CONTEXT_FLAG_DEBUG_BIT) != 0 ? "yes"
                                                                  : "no")
            << '\n';

  const bool requested_version_met =
      actual_major > requested_major ||
      (actual_major == requested_major && actual_minor >= requested_minor);
  const bool core_profile =
      (profile_mask & GL_CONTEXT_CORE_PROFILE_BIT) != 0;

  SDL_GL_DestroyContext(context);
  SDL_DestroyWindow(window);
  quit_sdl();

  if (!requested_version_met || !core_profile) {
    std::cerr << "result=fail\n"
              << "failure_stage=context_validation\n";
    return EXIT_FAILURE;
  }

  std::cout << "result=pass\n";
  return EXIT_SUCCESS;
}
