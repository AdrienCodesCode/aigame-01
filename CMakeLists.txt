cmake_minimum_required(VERSION 3.28)

project(WideEye VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
include(cmake/WideEyeDependencies.cmake)
include(cmake/WideEyeDeveloperTools.cmake)
include(cmake/WideEyeProjectOptions.cmake)

set(
    wide_eye_sources
    "${PROJECT_SOURCE_DIR}/src/core/runtime.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/main.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/window_state.cpp"
    "${PROJECT_SOURCE_DIR}/src/render/triangle_renderer.cpp"
)
set(wide_eye_capture_sources "${PROJECT_SOURCE_DIR}/src/render/png_writer.cpp")
set(wide_eye_test_sources "${PROJECT_SOURCE_DIR}/tests/png_writer_tests.cpp")
set(
    wide_eye_headers
    "${PROJECT_SOURCE_DIR}/src/core/runtime.hpp"
    "${PROJECT_SOURCE_DIR}/src/platform/window_state.hpp"
    "${PROJECT_SOURCE_DIR}/src/render/triangle_renderer.hpp"
)
set(wide_eye_capture_headers "${PROJECT_SOURCE_DIR}/src/render/png_writer.hpp")

add_library(wide_eye_render_capture STATIC ${wide_eye_capture_sources} ${wide_eye_capture_headers})
target_compile_features(wide_eye_render_capture PUBLIC cxx_std_23)
target_include_directories(wide_eye_render_capture PUBLIC "${PROJECT_SOURCE_DIR}/src")
set_target_properties(
    wide_eye_render_capture
    PROPERTIES CXX_EXTENSIONS OFF
               CXX_SCAN_FOR_MODULES OFF
               CXX_STANDARD_REQUIRED ON
)
wide_eye_set_project_options(wide_eye_render_capture)

add_executable(wide_eye ${wide_eye_sources} ${wide_eye_headers})
target_compile_features(wide_eye PRIVATE cxx_std_23)
target_compile_definitions(wide_eye PRIVATE WIDE_EYE_VERSION="${PROJECT_VERSION}")
target_include_directories(wide_eye PRIVATE "${PROJECT_SOURCE_DIR}/src")
set_target_properties(
    wide_eye
    PROPERTIES CXX_EXTENSIONS OFF
               CXX_SCAN_FOR_MODULES OFF
               CXX_STANDARD_REQUIRED ON
)
wide_eye_set_project_options(wide_eye)
wide_eye_add_developer_checks(
    SOURCES ${wide_eye_sources} ${wide_eye_capture_sources} ${wide_eye_test_sources}
    HEADERS ${wide_eye_headers} ${wide_eye_capture_headers}
)
target_link_libraries(wide_eye PRIVATE SDL3::SDL3 wide_eye_render_capture)

set(wide_eye_context_test_default OFF)
if(WIN32)
    set(wide_eye_context_test_default ON)
endif()
option(
    WIDE_EYE_ENABLE_OPENGL_CONTEXT_TEST
    "Run the display-backed OpenGL 4.6 Core context smoke through CTest"
    ${wide_eye_context_test_default}
)

if(WIN32 AND TARGET SDL3::SDL3-shared)
    add_custom_command(
        TARGET wide_eye
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3-shared>
            $<TARGET_FILE_DIR:wide_eye>
        VERBATIM
    )
endif()

if(BUILD_TESTING)
    add_executable(wide_eye_png_writer_tests ${wide_eye_test_sources})
    target_compile_features(wide_eye_png_writer_tests PRIVATE cxx_std_23)
    set_target_properties(
        wide_eye_png_writer_tests
        PROPERTIES CXX_EXTENSIONS OFF
                   CXX_SCAN_FOR_MODULES OFF
                   CXX_STANDARD_REQUIRED ON
    )
    wide_eye_set_project_options(wide_eye_png_writer_tests)
    target_link_libraries(wide_eye_png_writer_tests PRIVATE wide_eye_render_capture)

    add_test(NAME wide_eye.scaffold_version COMMAND wide_eye --version)
    add_test(
        NAME wide_eye.png_writer
        COMMAND wide_eye_png_writer_tests "${CMAKE_CURRENT_BINARY_DIR}/png-writer-test.png"
    )
    add_test(NAME wide_eye.core_runtime COMMAND wide_eye --runtime-smoke)
    add_test(NAME wide_eye.window_state COMMAND wide_eye --window-state-smoke)
    add_test(
        NAME wide_eye.ctest_failure_regex
        COMMAND
            ${CMAKE_COMMAND}
            "-DCMAKE_CTEST_COMMAND=${CMAKE_CTEST_COMMAND}"
            "-DFIXTURE_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/ctest-failure-regex-fixture"
            "-DWIDE_EYE_FAILURE_EXPRESSION=failure_stage=|ERROR: AddressSanitizer|ERROR: LeakSanitizer|runtime error:"
            -P
            "${PROJECT_SOURCE_DIR}/tests/assert-ctest-failure-regex.cmake"
    )
    add_test(
        NAME wide_eye.core_assertion
        COMMAND
            ${CMAKE_COMMAND}
            "-DWIDE_EYE_EXECUTABLE=$<TARGET_FILE:wide_eye>"
            -P
            "${PROJECT_SOURCE_DIR}/tests/assert-core-assertion.cmake"
    )
    add_test(NAME wide_eye.sdl_window_smoke COMMAND wide_eye --window-smoke)
    add_test(
        NAME wide_eye.accepted_tracer0_baseline
        COMMAND
            ${CMAKE_COMMAND}
            "-DMANIFEST=${PROJECT_SOURCE_DIR}/tests/goldens/tracer0/voxel_cube_smoke-v1/windows-intel-uhd-630-development/manifest.json"
            -DEXPECTED_MANIFEST_SHA256=378d2c8cf2a5ec02e5b543311b70bf1a935e4191b24f7cee1b0a6ce84f0b8634
            -DEXPECTED_REVIEW_SHA256=a0b3453cecde5d2fe684b5635c70cab5686ff7cc07786b953dab63c12e74af6c
            -DEXPECTED_RESULT=pass
            -DREQUIRE_CAPTURE=ON
            -DREQUIRE_DEBUG_CAPTURE=ON
            -DREQUIRE_REVIEW=ON
            -DREQUIRE_ACCEPTED_REVIEW=ON
            -P
            "${PROJECT_SOURCE_DIR}/tests/assert-artifact-manifest.cmake"
    )
    if(WIDE_EYE_ENABLE_OPENGL_CONTEXT_TEST)
        add_test(NAME wide_eye.opengl_context_smoke COMMAND wide_eye --context-smoke)
        add_test(NAME wide_eye.opengl_triangle_smoke COMMAND wide_eye --triangle-smoke)
        add_test(NAME wide_eye.opengl_voxel_cube_smoke COMMAND wide_eye --voxel-cube-smoke)
        add_test(
            NAME wide_eye.opengl_voxel_cube_debug_smoke
            COMMAND wide_eye --voxel-cube-debug-smoke
        )
        add_test(
            NAME wide_eye.opengl_voxel_cube_capture
            COMMAND
                ${CMAKE_COMMAND}
                "-DWIDE_EYE_EXECUTABLE=$<TARGET_FILE:wide_eye>"
                "-DCAPTURE_ONE=${CMAKE_CURRENT_BINARY_DIR}/voxel-cube-capture-one.png"
                "-DCAPTURE_TWO=${CMAKE_CURRENT_BINARY_DIR}/voxel-cube-capture-two.png"
                -P
                "${PROJECT_SOURCE_DIR}/tests/assert-deterministic-png.cmake"
        )
        add_test(
            NAME wide_eye.opengl_voxel_cube_debug_capture
            COMMAND
                ${CMAKE_COMMAND}
                "-DWIDE_EYE_EXECUTABLE=$<TARGET_FILE:wide_eye>"
                "-DWIDE_EYE_SMOKE_ARGUMENT=--voxel-cube-debug-smoke"
                "-DWIDE_EYE_RESULT_MARKER=voxel_cube_debug_result=pass"
                "-DCAPTURE_ONE=${CMAKE_CURRENT_BINARY_DIR}/voxel-cube-debug-capture-one.png"
                "-DCAPTURE_TWO=${CMAKE_CURRENT_BINARY_DIR}/voxel-cube-debug-capture-two.png"
                -P
                "${PROJECT_SOURCE_DIR}/tests/assert-deterministic-png.cmake"
        )
        add_test(
            NAME wide_eye.opengl_debug_high_severity
            COMMAND
                ${CMAKE_COMMAND}
                "-DWIDE_EYE_EXECUTABLE=$<TARGET_FILE:wide_eye>"
                -P
                "${PROJECT_SOURCE_DIR}/tests/assert-opengl-high-severity.cmake"
        )
    endif()

    set(wide_eye_scaffold_labels headless)
    set(wide_eye_png_writer_labels unit)
    set(wide_eye_core_runtime_labels unit)
    set(wide_eye_window_state_labels unit)
    set(wide_eye_ctest_failure_regex_labels unit headless)
    set(wide_eye_core_assertion_labels unit)
    set(wide_eye_window_smoke_labels scenario headless)
    set(wide_eye_accepted_tracer0_baseline_labels scenario headless)
    set(wide_eye_context_smoke_labels scenario headless)
    set(wide_eye_triangle_smoke_labels scenario headless)
    set(wide_eye_voxel_cube_smoke_labels scenario headless)
    set(wide_eye_voxel_cube_debug_smoke_labels scenario headless)
    set(wide_eye_voxel_cube_capture_labels scenario headless)
    set(wide_eye_voxel_cube_debug_capture_labels scenario headless)
    set(wide_eye_gl_debug_labels scenario headless)
    if(WIDE_EYE_ENABLE_SANITIZERS)
        list(APPEND wide_eye_scaffold_labels sanitizer)
        list(APPEND wide_eye_png_writer_labels sanitizer)
        list(APPEND wide_eye_core_runtime_labels sanitizer)
        list(APPEND wide_eye_window_state_labels sanitizer)
        list(APPEND wide_eye_core_assertion_labels sanitizer)
        list(APPEND wide_eye_window_smoke_labels sanitizer)
        list(APPEND wide_eye_accepted_tracer0_baseline_labels sanitizer)
        list(APPEND wide_eye_context_smoke_labels sanitizer)
        list(APPEND wide_eye_triangle_smoke_labels sanitizer)
        list(APPEND wide_eye_voxel_cube_smoke_labels sanitizer)
        list(APPEND wide_eye_voxel_cube_debug_smoke_labels sanitizer)
        list(APPEND wide_eye_voxel_cube_capture_labels sanitizer)
        list(APPEND wide_eye_voxel_cube_debug_capture_labels sanitizer)
        list(APPEND wide_eye_gl_debug_labels sanitizer)
    endif()

    set_tests_properties(
        wide_eye.scaffold_version
        PROPERTIES LABELS "${wide_eye_scaffold_labels}"
                   PASS_REGULAR_EXPRESSION "Wide Eye ${PROJECT_VERSION}"
    )
    set_tests_properties(
        wide_eye.png_writer
        PROPERTIES LABELS "${wide_eye_png_writer_labels}"
                   PASS_REGULAR_EXPRESSION "png_writer_result=pass"
                   TIMEOUT 5
    )
    set_tests_properties(
        wide_eye.core_runtime
        PROPERTIES LABELS "${wide_eye_core_runtime_labels}"
                   PASS_REGULAR_EXPRESSION "runtime_result=pass"
                   TIMEOUT 5
    )
    set_tests_properties(
        wide_eye.window_state
        PROPERTIES LABELS "${wide_eye_window_state_labels}"
                   PASS_REGULAR_EXPRESSION "window_state_result=pass"
                   TIMEOUT 5
    )
    set_tests_properties(
        wide_eye.ctest_failure_regex
        PROPERTIES LABELS "${wide_eye_ctest_failure_regex_labels}"
                   PASS_REGULAR_EXPRESSION "ctest_failure_regex_result=pass"
                   TIMEOUT 5
    )
    set_tests_properties(
        wide_eye.core_assertion
        PROPERTIES LABELS "${wide_eye_core_assertion_labels}"
                   TIMEOUT 5
    )
    set_tests_properties(
        wide_eye.sdl_window_smoke
        PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy"
                   LABELS "${wide_eye_window_smoke_labels}"
                   PASS_REGULAR_EXPRESSION "window_result=pass"
                   TIMEOUT 5
    )
    set_tests_properties(
        wide_eye.accepted_tracer0_baseline
        PROPERTIES LABELS "${wide_eye_accepted_tracer0_baseline_labels}"
                   PASS_REGULAR_EXPRESSION "artifact_manifest_result=pass"
                   TIMEOUT 5
    )
    if(WIDE_EYE_ENABLE_OPENGL_CONTEXT_TEST)
        set_tests_properties(
            wide_eye.opengl_context_smoke
            PROPERTIES LABELS "${wide_eye_context_smoke_labels}"
                       PASS_REGULAR_EXPRESSION "context_result=pass"
                       TIMEOUT 5
        )
        set_tests_properties(
            wide_eye.opengl_triangle_smoke
            PROPERTIES LABELS "${wide_eye_triangle_smoke_labels}"
                       PASS_REGULAR_EXPRESSION "triangle_result=pass"
                       TIMEOUT 5
        )
        set_tests_properties(
            wide_eye.opengl_voxel_cube_smoke
            PROPERTIES LABELS "${wide_eye_voxel_cube_smoke_labels}"
                       PASS_REGULAR_EXPRESSION "voxel_cube_result=pass"
                       TIMEOUT 5
        )
        set_tests_properties(
            wide_eye.opengl_voxel_cube_debug_smoke
            PROPERTIES LABELS "${wide_eye_voxel_cube_debug_smoke_labels}"
                       PASS_REGULAR_EXPRESSION "wireframe_frame_matches=yes"
                       TIMEOUT 5
        )
        set_tests_properties(
            wide_eye.opengl_voxel_cube_capture
            PROPERTIES LABELS "${wide_eye_voxel_cube_capture_labels}"
                       PASS_REGULAR_EXPRESSION "capture_repeat_sha256="
                       TIMEOUT 10
        )
        set_tests_properties(
            wide_eye.opengl_voxel_cube_debug_capture
            PROPERTIES LABELS "${wide_eye_voxel_cube_debug_capture_labels}"
                       PASS_REGULAR_EXPRESSION "capture_repeat_sha256="
                       TIMEOUT 10
        )
        set_tests_properties(
            wide_eye.opengl_debug_high_severity
            PROPERTIES LABELS "${wide_eye_gl_debug_labels}"
                       TIMEOUT 5
        )
    endif()

    set(wide_eye_registered_tests
        wide_eye.scaffold_version
        wide_eye.png_writer
        wide_eye.core_runtime
        wide_eye.window_state
        wide_eye.ctest_failure_regex
        wide_eye.core_assertion
        wide_eye.sdl_window_smoke
        wide_eye.accepted_tracer0_baseline
    )
    if(WIDE_EYE_ENABLE_OPENGL_CONTEXT_TEST)
        list(
            APPEND
            wide_eye_registered_tests
            wide_eye.opengl_context_smoke
            wide_eye.opengl_triangle_smoke
            wide_eye.opengl_voxel_cube_smoke
            wide_eye.opengl_voxel_cube_debug_smoke
            wide_eye.opengl_voxel_cube_capture
            wide_eye.opengl_voxel_cube_debug_capture
            wide_eye.opengl_debug_high_severity
        )
    endif()
    set_tests_properties(
        ${wide_eye_registered_tests}
        PROPERTIES FAIL_REGULAR_EXPRESSION
                   "failure_stage=|ERROR: AddressSanitizer|ERROR: LeakSanitizer|runtime error:"
    )
endif()
