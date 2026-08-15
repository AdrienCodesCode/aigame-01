include_guard(GLOBAL)

include(FetchContent)

set(SDL_SHARED ON CACHE BOOL "Build SDL as a shared library" FORCE)
set(SDL_STATIC OFF CACHE BOOL "Do not build a second static SDL library" FORCE)
set(SDL_TESTS OFF CACHE BOOL "Do not build SDL's own test suite" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "Do not build SDL's test support library" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "Do not build SDL examples" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "Do not add SDL install rules to the game build" FORCE)
set(SDL_UNINSTALL OFF CACHE BOOL "Do not add SDL uninstall rules to the game build" FORCE)

# Tracer 0 needs native windowing and events only. Enable later SDL subsystems
# when an accepted engine outcome owns and verifies them.
set(SDL_AUDIO OFF CACHE BOOL "Audio is outside the current tracer" FORCE)
set(SDL_CAMERA OFF CACHE BOOL "Camera capture is outside the current tracer" FORCE)
set(SDL_DIALOG OFF CACHE BOOL "Native dialogs are outside the current tracer" FORCE)
set(SDL_GPU OFF CACHE BOOL "The engine uses its own OpenGL renderer" FORCE)
set(SDL_HAPTIC OFF CACHE BOOL "Haptics are outside the current tracer" FORCE)
set(SDL_JOYSTICK OFF CACHE BOOL "Controller input is outside the current tracer" FORCE)
set(SDL_RENDER OFF CACHE BOOL "The engine uses its own OpenGL renderer" FORCE)
set(SDL_SENSOR OFF CACHE BOOL "Sensors are outside the current tracer" FORCE)
set(SDL_TRAY OFF CACHE BOOL "Tray integration is outside the current tracer" FORCE)
set(SDL_WAYLAND OFF CACHE BOOL "The current Linux development path uses X11" FORCE)
set(SDL_KMSDRM OFF CACHE BOOL "Direct KMS/DRM is outside the current tracer" FORCE)

FetchContent_Declare(
    SDL3
    URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.10/SDL3-3.4.10.tar.gz
    URL_HASH SHA256=12b34280415ec8418c864408b93d008a20a6530687ee613d60bfbd20411f2785
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(SDL3)

if(NOT TARGET SDL3::SDL3)
    message(FATAL_ERROR "Pinned SDL 3.4.10 did not provide the SDL3::SDL3 target")
endif()
