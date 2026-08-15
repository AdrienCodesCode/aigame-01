cmake_minimum_required(VERSION 3.28)

project(wide_eye_phase0_context_smoke VERSION 0.1.0 LANGUAGES C CXX)

include(FetchContent)

find_package(SDL3 3.4.10 EXACT CONFIG QUIET)

if(NOT TARGET SDL3::SDL3)
  set(SDL_SHARED ON CACHE BOOL "" FORCE)
  set(SDL_STATIC OFF CACHE BOOL "" FORCE)
  set(SDL_TESTS OFF CACHE BOOL "" FORCE)
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(SDL_AUDIO OFF CACHE BOOL "" FORCE)
  set(SDL_CAMERA OFF CACHE BOOL "" FORCE)
  set(SDL_DIALOG OFF CACHE BOOL "" FORCE)
  set(SDL_GPU OFF CACHE BOOL "" FORCE)
  set(SDL_HAPTIC OFF CACHE BOOL "" FORCE)
  set(SDL_JOYSTICK OFF CACHE BOOL "" FORCE)
  set(SDL_RENDER OFF CACHE BOOL "" FORCE)
  set(SDL_SENSOR OFF CACHE BOOL "" FORCE)
  set(SDL_TRAY OFF CACHE BOOL "" FORCE)
  set(SDL_WAYLAND OFF CACHE BOOL "" FORCE)
  set(SDL_KMSDRM OFF CACHE BOOL "" FORCE)

  FetchContent_Declare(
    SDL3
    URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.10/SDL3-3.4.10.tar.gz
    URL_HASH SHA256=12b34280415ec8418c864408b93d008a20a6530687ee613d60bfbd20411f2785
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(SDL3)
endif()

add_executable(wide-eye-context-smoke main.cpp)
target_compile_features(wide-eye-context-smoke PRIVATE cxx_std_23)
set_target_properties(
  wide-eye-context-smoke
  PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_SCAN_FOR_MODULES OFF
)
target_compile_options(
  wide-eye-context-smoke
  PRIVATE
    $<$<CXX_COMPILER_ID:Clang,GNU>:-Wall;-Wextra;-Wpedantic;-Wconversion;-Wsign-conversion>
    $<$<CXX_COMPILER_ID:MSVC>:/W4;/permissive->
)
target_link_libraries(wide-eye-context-smoke PRIVATE SDL3::SDL3)

if(WIN32 AND TARGET SDL3::SDL3-shared)
  add_custom_command(
    TARGET wide-eye-context-smoke
    POST_BUILD
    COMMAND
      ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:SDL3::SDL3-shared>
      $<TARGET_FILE_DIR:wide-eye-context-smoke>
    VERBATIM
  )
endif()
