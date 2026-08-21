cmake_minimum_required(VERSION 3.28)

project(WideEye VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
include(cmake/WideEyeDependencies.cmake)
include(cmake/WideEyeDeveloperTools.cmake)
include(cmake/WideEyeProjectOptions.cmake)

set(
    wide_eye_sources
    "${PROJECT_SOURCE_DIR}/src/platform/main.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/scenario_runner.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/window_runtime.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/window_state.cpp"
    "${PROJECT_SOURCE_DIR}/src/render/opengl_renderer.cpp"
    "${PROJECT_SOURCE_DIR}/src/render/sheep_proxy.cpp"
)
set(wide_eye_core_runtime_sources "${PROJECT_SOURCE_DIR}/src/core/runtime.cpp")
set(wide_eye_core_runtime_headers "${PROJECT_SOURCE_DIR}/src/core/runtime.hpp")
set(wide_eye_core_performance_sources "${PROJECT_SOURCE_DIR}/src/core/performance.cpp")
set(wide_eye_core_performance_headers "${PROJECT_SOURCE_DIR}/src/core/performance.hpp")
set(
    wide_eye_game_sources
    "${PROJECT_SOURCE_DIR}/src/game/camera_controller.cpp"
    "${PROJECT_SOURCE_DIR}/src/game/dog_controller.cpp"
    "${PROJECT_SOURCE_DIR}/src/game/flock_observables.cpp"
    "${PROJECT_SOURCE_DIR}/src/game/gameplay_replay.cpp"
    "${PROJECT_SOURCE_DIR}/src/game/gameplay_scenario.cpp"
    "${PROJECT_SOURCE_DIR}/src/game/gameplay_simulation.cpp"
    "${PROJECT_SOURCE_DIR}/src/game/paddock_collision.cpp"
    "${PROJECT_SOURCE_DIR}/src/game/sheep_spatial_grid.cpp"
)
set(
    wide_eye_game_headers
    "${PROJECT_SOURCE_DIR}/src/game/camera_controller.hpp"
    "${PROJECT_SOURCE_DIR}/src/game/dog_controller.hpp"
    "${PROJECT_SOURCE_DIR}/src/game/flock_observables.hpp"
    "${PROJECT_SOURCE_DIR}/src/game/gameplay_replay.hpp"
    "${PROJECT_SOURCE_DIR}/src/game/gameplay_scenario.hpp"
    "${PROJECT_SOURCE_DIR}/src/game/gameplay_simulation.hpp"
    "${PROJECT_SOURCE_DIR}/src/game/math.hpp"
    "${PROJECT_SOURCE_DIR}/src/game/paddock_collision.hpp"
    "${PROJECT_SOURCE_DIR}/src/game/sheep_spatial_grid.hpp"
    "${PROJECT_SOURCE_DIR}/src/game/sheep_state.hpp"
)
set(wide_eye_platform_input_sources "${PROJECT_SOURCE_DIR}/src/platform/input.cpp")
set(wide_eye_platform_input_headers "${PROJECT_SOURCE_DIR}/src/platform/input.hpp")
set(wide_eye_capture_sources "${PROJECT_SOURCE_DIR}/src/render/png_writer.cpp")
set(wide_eye_test_sources "${PROJECT_SOURCE_DIR}/tests/png_writer_tests.cpp")
set(
    wide_eye_voxel_sources
    "${PROJECT_SOURCE_DIR}/src/voxel/chunk.cpp"
    "${PROJECT_SOURCE_DIR}/src/voxel/coordinates.cpp"
    "${PROJECT_SOURCE_DIR}/src/voxel/handcrafted_paddock.cpp"
    "${PROJECT_SOURCE_DIR}/src/voxel/naive_mesher.cpp"
)
set(wide_eye_voxel_test_sources "${PROJECT_SOURCE_DIR}/tests/voxel_coordinates_tests.cpp")
set(wide_eye_chunk_storage_test_sources "${PROJECT_SOURCE_DIR}/tests/chunk_storage_tests.cpp")
set(
    wide_eye_handcrafted_paddock_test_sources
    "${PROJECT_SOURCE_DIR}/tests/handcrafted_paddock_tests.cpp"
)
set(wide_eye_naive_mesher_test_sources "${PROJECT_SOURCE_DIR}/tests/naive_mesher_tests.cpp")
set(
    wide_eye_chunk_size_comparison_sources
    "${PROJECT_SOURCE_DIR}/tests/chunk_size_comparison.cpp"
)
set(wide_eye_dog_test_sources "${PROJECT_SOURCE_DIR}/tests/dog_controller_tests.cpp")
set(
    wide_eye_gameplay_simulation_test_sources
    "${PROJECT_SOURCE_DIR}/tests/gameplay_simulation_tests.cpp"
)
set(wide_eye_flock_observables_test_sources "${PROJECT_SOURCE_DIR}/tests/flock_observables_tests.cpp")
set(wide_eye_sheep_spatial_grid_test_sources "${PROJECT_SOURCE_DIR}/tests/sheep_spatial_grid_tests.cpp")
set(wide_eye_input_test_sources "${PROJECT_SOURCE_DIR}/tests/input_tests.cpp")
set(wide_eye_performance_test_sources "${PROJECT_SOURCE_DIR}/tests/performance_tests.cpp")
set(wide_eye_sheep_proxy_test_sources "${PROJECT_SOURCE_DIR}/tests/sheep_proxy_tests.cpp")
set(
    wide_eye_headers
    "${PROJECT_SOURCE_DIR}/src/platform/scenario_runner.hpp"
    "${PROJECT_SOURCE_DIR}/src/platform/window_runtime.hpp"
    "${PROJECT_SOURCE_DIR}/src/platform/window_state.hpp"
    "${PROJECT_SOURCE_DIR}/src/render/opengl_renderer.hpp"
    "${PROJECT_SOURCE_DIR}/src/render/sheep_proxy.hpp"
)
set(wide_eye_capture_headers "${PROJECT_SOURCE_DIR}/src/render/png_writer.hpp")
set(
    wide_eye_voxel_headers
    "${PROJECT_SOURCE_DIR}/src/voxel/chunk.hpp"
    "${PROJECT_SOURCE_DIR}/src/voxel/coordinates.hpp"
    "${PROJECT_SOURCE_DIR}/src/voxel/handcrafted_paddock.hpp"
    "${PROJECT_SOURCE_DIR}/src/voxel/naive_mesher.hpp"
)

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

add_library(wide_eye_voxel STATIC ${wide_eye_voxel_sources} ${wide_eye_voxel_headers})
target_compile_features(wide_eye_voxel PUBLIC cxx_std_23)
target_include_directories(wide_eye_voxel PUBLIC "${PROJECT_SOURCE_DIR}/src")
set_target_properties(
    wide_eye_voxel
    PROPERTIES CXX_EXTENSIONS OFF
               CXX_SCAN_FOR_MODULES OFF
               CXX_STANDARD_REQUIRED ON
)
wide_eye_set_project_options(wide_eye_voxel)

add_library(
    wide_eye_core_runtime STATIC
    ${wide_eye_core_runtime_sources}
    ${wide_eye_core_runtime_headers}
)
target_compile_features(wide_eye_core_runtime PUBLIC cxx_std_23)
target_include_directories(wide_eye_core_runtime PUBLIC "${PROJECT_SOURCE_DIR}/src")
set_target_properties(
    wide_eye_core_runtime
    PROPERTIES CXX_EXTENSIONS OFF
               CXX_SCAN_FOR_MODULES OFF
               CXX_STANDARD_REQUIRED ON
)
wide_eye_set_project_options(wide_eye_core_runtime)

add_library(
    wide_eye_core_performance STATIC
    ${wide_eye_core_performance_sources}
    ${wide_eye_core_performance_headers}
)
target_compile_features(wide_eye_core_performance PUBLIC cxx_std_23)
target_include_directories(wide_eye_core_performance PUBLIC "${PROJECT_SOURCE_DIR}/src")
set_target_properties(
    wide_eye_core_performance
    PROPERTIES CXX_EXTENSIONS OFF
               CXX_SCAN_FOR_MODULES OFF
               CXX_STANDARD_REQUIRED ON
)
wide_eye_set_project_options(wide_eye_core_performance)
if(WIN32)
    target_link_libraries(wide_eye_core_performance PRIVATE Psapi)
endif()

add_library(wide_eye_game STATIC ${wide_eye_game_sources} ${wide_eye_game_headers})
target_compile_features(wide_eye_game PUBLIC cxx_std_23)
target_include_directories(wide_eye_game PUBLIC "${PROJECT_SOURCE_DIR}/src")
set_target_properties(
    wide_eye_game
    PROPERTIES CXX_EXTENSIONS OFF
               CXX_SCAN_FOR_MODULES OFF
               CXX_STANDARD_REQUIRED ON
)
wide_eye_set_project_options(wide_eye_game)
target_link_libraries(wide_eye_game PUBLIC wide_eye_core_runtime)

add_library(
    wide_eye_platform_input STATIC
    ${wide_eye_platform_input_sources}
    ${wide_eye_platform_input_headers}
)
target_compile_features(wide_eye_platform_input PUBLIC cxx_std_23)
target_include_directories(wide_eye_platform_input PUBLIC "${PROJECT_SOURCE_DIR}/src")
set_target_properties(
    wide_eye_platform_input
    PROPERTIES CXX_EXTENSIONS OFF
               CXX_SCAN_FOR_MODULES OFF
               CXX_STANDARD_REQUIRED ON
)
wide_eye_set_project_options(wide_eye_platform_input)
target_link_libraries(wide_eye_platform_input PUBLIC SDL3::SDL3)

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
    SOURCES ${wide_eye_sources} ${wide_eye_capture_sources} ${wide_eye_voxel_sources}
            ${wide_eye_core_runtime_sources} ${wide_eye_core_performance_sources}
            ${wide_eye_game_sources}
            ${wide_eye_platform_input_sources}
            ${wide_eye_test_sources} ${wide_eye_voxel_test_sources}
            ${wide_eye_chunk_storage_test_sources} ${wide_eye_handcrafted_paddock_test_sources}
            ${wide_eye_naive_mesher_test_sources}
            ${wide_eye_chunk_size_comparison_sources}
            ${wide_eye_dog_test_sources} ${wide_eye_gameplay_simulation_test_sources}
            ${wide_eye_flock_observables_test_sources}
            ${wide_eye_sheep_spatial_grid_test_sources}
            ${wide_eye_input_test_sources}
            ${wide_eye_performance_test_sources}
            ${wide_eye_sheep_proxy_test_sources}
    HEADERS ${wide_eye_headers} ${wide_eye_capture_headers} ${wide_eye_voxel_headers}
            ${wide_eye_core_runtime_headers} ${wide_eye_core_performance_headers}
            ${wide_eye_game_headers}
            ${wide_eye_platform_input_headers}
)
wide_eye_add_qa_tracker_targets()
target_link_libraries(
    wide_eye
    PRIVATE SDL3::SDL3 glad::glad wide_eye_core_runtime wide_eye_core_performance wide_eye_game
            wide_eye_platform_input wide_eye_render_capture wide_eye_voxel
)

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
    # Stack the gameplay-simulation harness must finish inside, checked by
    # wide_eye.gameplay_simulation_stack_budget. It protects against the defect
    # recorded in QA-002: fixtures held by value in one `main` grew the frame to
    # 7,400 KiB against the usual 8,192 KiB default, and the next fixture killed
    # the process with SIGSEGV and no output at all. Holding every fixture on the
    # heap took the requirement to 185 KiB (`dev`), 160 KiB (`release`) and
    # 220 KiB (`dev-sanitized`), measured on WSL Ubuntu 24.04.4 with Clang 18.1.3
    # on 2026-08-22. The budget is more than twice the largest of those, so
    # ordinary toolchain differences cannot make it flaky, and it is a sixteenth
    # of the usual default, so a fixture that moves back into a stack frame —
    # about 115 KiB each — fails here by name within two or three additions
    # instead of crashing silently much later.
    set(WIDE_EYE_GAMEPLAY_SIMULATION_STACK_BUDGET_KIB 512)

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

    add_executable(wide_eye_voxel_coordinates_tests ${wide_eye_voxel_test_sources})
    target_compile_features(wide_eye_voxel_coordinates_tests PRIVATE cxx_std_23)
    set_target_properties(
        wide_eye_voxel_coordinates_tests
        PROPERTIES CXX_EXTENSIONS OFF
                   CXX_SCAN_FOR_MODULES OFF
                   CXX_STANDARD_REQUIRED ON
    )
    wide_eye_set_project_options(wide_eye_voxel_coordinates_tests)
    target_link_libraries(wide_eye_voxel_coordinates_tests PRIVATE wide_eye_voxel)

    add_executable(wide_eye_chunk_storage_tests ${wide_eye_chunk_storage_test_sources})
    target_compile_features(wide_eye_chunk_storage_tests PRIVATE cxx_std_23)
    set_target_properties(
        wide_eye_chunk_storage_tests
        PROPERTIES CXX_EXTENSIONS OFF
                   CXX_SCAN_FOR_MODULES OFF
                   CXX_STANDARD_REQUIRED ON
    )
    wide_eye_set_project_options(wide_eye_chunk_storage_tests)
    target_link_libraries(wide_eye_chunk_storage_tests PRIVATE wide_eye_voxel)

    add_executable(
        wide_eye_handcrafted_paddock_tests
        ${wide_eye_handcrafted_paddock_test_sources}
    )
    target_compile_features(wide_eye_handcrafted_paddock_tests PRIVATE cxx_std_23)
    set_target_properties(
        wide_eye_handcrafted_paddock_tests
        PROPERTIES CXX_EXTENSIONS OFF
                   CXX_SCAN_FOR_MODULES OFF
                   CXX_STANDARD_REQUIRED ON
    )
    wide_eye_set_project_options(wide_eye_handcrafted_paddock_tests)
    target_link_libraries(wide_eye_handcrafted_paddock_tests PRIVATE wide_eye_voxel)

    add_executable(wide_eye_naive_mesher_tests ${wide_eye_naive_mesher_test_sources})
    target_compile_features(wide_eye_naive_mesher_tests PRIVATE cxx_std_23)
    set_target_properties(
        wide_eye_naive_mesher_tests
        PROPERTIES CXX_EXTENSIONS OFF
                   CXX_SCAN_FOR_MODULES OFF
                   CXX_STANDARD_REQUIRED ON
    )
    wide_eye_set_project_options(wide_eye_naive_mesher_tests)
    target_link_libraries(wide_eye_naive_mesher_tests PRIVATE wide_eye_voxel)

    add_executable(
        wide_eye_chunk_size_comparison
        ${wide_eye_chunk_size_comparison_sources}
    )
    target_compile_features(wide_eye_chunk_size_comparison PRIVATE cxx_std_23)
    set_target_properties(
        wide_eye_chunk_size_comparison
        PROPERTIES CXX_EXTENSIONS OFF
                   CXX_SCAN_FOR_MODULES OFF
                   CXX_STANDARD_REQUIRED ON
    )
    wide_eye_set_project_options(wide_eye_chunk_size_comparison)
    target_link_libraries(wide_eye_chunk_size_comparison PRIVATE wide_eye_voxel)

    add_executable(wide_eye_dog_controller_tests ${wide_eye_dog_test_sources})
    target_compile_features(wide_eye_dog_controller_tests PRIVATE cxx_std_23)
    set_target_properties(
        wide_eye_dog_controller_tests
        PROPERTIES CXX_EXTENSIONS OFF
                   CXX_SCAN_FOR_MODULES OFF
                   CXX_STANDARD_REQUIRED ON
    )
    wide_eye_set_project_options(wide_eye_dog_controller_tests)
    target_link_libraries(wide_eye_dog_controller_tests PRIVATE wide_eye_game)

    add_executable(
        wide_eye_gameplay_simulation_tests
        ${wide_eye_gameplay_simulation_test_sources}
    )
    target_compile_features(wide_eye_gameplay_simulation_tests PRIVATE cxx_std_23)
    set_target_properties(
        wide_eye_gameplay_simulation_tests
        PROPERTIES CXX_EXTENSIONS OFF
                   CXX_SCAN_FOR_MODULES OFF
                   CXX_STANDARD_REQUIRED ON
    )
    wide_eye_set_project_options(wide_eye_gameplay_simulation_tests)
    target_link_libraries(
        wide_eye_gameplay_simulation_tests
        PRIVATE wide_eye_core_runtime wide_eye_game
    )

    add_executable(wide_eye_flock_observables_tests ${wide_eye_flock_observables_test_sources})
    target_compile_features(wide_eye_flock_observables_tests PRIVATE cxx_std_23)
    set_target_properties(
        wide_eye_flock_observables_tests
        PROPERTIES CXX_EXTENSIONS OFF
                   CXX_SCAN_FOR_MODULES OFF
                   CXX_STANDARD_REQUIRED ON
    )
    wide_eye_set_project_options(wide_eye_flock_observables_tests)
    target_link_libraries(wide_eye_flock_observables_tests PRIVATE wide_eye_game)

    add_executable(wide_eye_sheep_spatial_grid_tests ${wide_eye_sheep_spatial_grid_test_sources})
    target_compile_features(wide_eye_sheep_spatial_grid_tests PRIVATE cxx_std_23)
    set_target_properties(
        wide_eye_sheep_spatial_grid_tests
        PROPERTIES CXX_EXTENSIONS OFF
                   CXX_SCAN_FOR_MODULES OFF
                   CXX_STANDARD_REQUIRED ON
    )
    wide_eye_set_project_options(wide_eye_sheep_spatial_grid_tests)
    target_link_libraries(wide_eye_sheep_spatial_grid_tests PRIVATE wide_eye_game)

    add_executable(wide_eye_input_tests ${wide_eye_input_test_sources})
    target_compile_features(wide_eye_input_tests PRIVATE cxx_std_23)
    set_target_properties(
        wide_eye_input_tests
        PROPERTIES CXX_EXTENSIONS OFF
                   CXX_SCAN_FOR_MODULES OFF
                   CXX_STANDARD_REQUIRED ON
    )
    wide_eye_set_project_options(wide_eye_input_tests)
    target_link_libraries(wide_eye_input_tests PRIVATE wide_eye_platform_input)

    add_executable(wide_eye_performance_tests ${wide_eye_performance_test_sources})
    target_compile_features(wide_eye_performance_tests PRIVATE cxx_std_23)
    set_target_properties(
        wide_eye_performance_tests
        PROPERTIES CXX_EXTENSIONS OFF
                   CXX_SCAN_FOR_MODULES OFF
                   CXX_STANDARD_REQUIRED ON
    )
    wide_eye_set_project_options(wide_eye_performance_tests)
    target_link_libraries(wide_eye_performance_tests PRIVATE wide_eye_core_performance)

    add_executable(
        wide_eye_sheep_proxy_tests
        ${wide_eye_sheep_proxy_test_sources}
        "${PROJECT_SOURCE_DIR}/src/render/sheep_proxy.cpp"
    )
    target_compile_features(wide_eye_sheep_proxy_tests PRIVATE cxx_std_23)
    set_target_properties(
        wide_eye_sheep_proxy_tests
        PROPERTIES CXX_EXTENSIONS OFF
                   CXX_SCAN_FOR_MODULES OFF
                   CXX_STANDARD_REQUIRED ON
    )
    wide_eye_set_project_options(wide_eye_sheep_proxy_tests)
    target_link_libraries(wide_eye_sheep_proxy_tests PRIVATE wide_eye_game)

    add_test(NAME wide_eye.scaffold_version COMMAND wide_eye --version)
    add_test(
        NAME wide_eye.png_writer
        COMMAND wide_eye_png_writer_tests "${CMAKE_CURRENT_BINARY_DIR}/png-writer-test.png"
    )
    add_test(NAME wide_eye.voxel_coordinates COMMAND wide_eye_voxel_coordinates_tests)
    add_test(NAME wide_eye.chunk_storage COMMAND wide_eye_chunk_storage_tests)
    add_test(NAME wide_eye.handcrafted_paddock COMMAND wide_eye_handcrafted_paddock_tests)
    add_test(NAME wide_eye.naive_mesher COMMAND wide_eye_naive_mesher_tests)
    add_test(
        NAME wide_eye.chunk_size_comparison
        COMMAND wide_eye_chunk_size_comparison --validate-only
    )
    add_test(NAME wide_eye.core_runtime COMMAND wide_eye --runtime-smoke)
    add_test(NAME wide_eye.window_state COMMAND wide_eye --window-state-smoke)
    add_test(NAME wide_eye.dog_controller COMMAND wide_eye_dog_controller_tests)
    add_test(NAME wide_eye.gameplay_simulation COMMAND wide_eye_gameplay_simulation_tests)
    if(UNIX)
        add_test(
            NAME wide_eye.gameplay_simulation_stack_budget
            COMMAND
                ${CMAKE_COMMAND}
                "-DWIDE_EYE_TEST_EXECUTABLE=$<TARGET_FILE:wide_eye_gameplay_simulation_tests>"
                "-DWIDE_EYE_STACK_BUDGET_KIB=${WIDE_EYE_GAMEPLAY_SIMULATION_STACK_BUDGET_KIB}"
                "-DWIDE_EYE_PASS_EXPRESSION=gameplay_simulation_result=pass"
                -P
                "${PROJECT_SOURCE_DIR}/tests/assert-stack-budget.cmake"
        )
    endif()
    add_test(NAME wide_eye.flock_observables COMMAND wide_eye_flock_observables_tests)
    add_test(NAME wide_eye.sheep_spatial_grid COMMAND wide_eye_sheep_spatial_grid_tests)
    add_test(NAME wide_eye.input_actions COMMAND wide_eye_input_tests)
    add_test(NAME wide_eye.performance_utilities COMMAND wide_eye_performance_tests)
    add_test(NAME wide_eye.sheep_proxy COMMAND wide_eye_sheep_proxy_tests)
    add_test(NAME wide_eye.dog_wall_scenario COMMAND wide_eye --dog-scenario wall-contact)
    add_test(NAME wide_eye.dog_closed_gate_scenario COMMAND wide_eye --dog-scenario closed-gate)
    add_test(NAME wide_eye.dog_open_gate_scenario COMMAND wide_eye --dog-scenario open-gate)
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
    add_test(
        NAME wide_eye.accepted_tracer1_paddock_baseline
        COMMAND
            ${CMAKE_COMMAND}
            "-DMANIFEST=${PROJECT_SOURCE_DIR}/tests/goldens/tracer1/handcrafted_paddock-v1/windows-intel-uhd-630-development-blockout/manifest.json"
            -DEXPECTED_MANIFEST_SHA256=b3825afde591210c855f3f779c822899d5311aeb1d2ea1f5557fd329ebd1ec93
            -DEXPECTED_REVIEW_SHA256=9fc3699938a1fcf656afdb69273a6edaadbcf5a2425429cff1ec3a010b8f9569
            -DEXPECTED_CAPTURE_SHA256=173238274346f39ce3a5fae87e2524e515cb65636302e0e2c3541cf0eaec92d2
            -P
            "${PROJECT_SOURCE_DIR}/tests/assert-handcrafted-paddock-baseline.cmake"
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
        add_test(NAME wide_eye.opengl_handcrafted_paddock COMMAND wide_eye --paddock-smoke)
        add_test(
            NAME wide_eye.opengl_dog_placeholder
            COMMAND wide_eye --dog-render-smoke paddock-start
        )
        add_test(
            NAME wide_eye.opengl_sheep_motion_fixture
            COMMAND wide_eye --sheep-motion-render-smoke
        )
        add_test(
            NAME wide_eye.opengl_handcrafted_paddock_chunk_bounds
            COMMAND wide_eye --paddock-chunk-bounds-smoke
        )
        add_test(
            NAME wide_eye.opengl_handcrafted_paddock_face_normals
            COMMAND wide_eye --paddock-face-normals-smoke
        )
        add_test(
            NAME wide_eye.opengl_handcrafted_paddock_wireframe
            COMMAND wide_eye --paddock-wireframe-smoke
        )
        add_test(
            NAME wide_eye.opengl_handcrafted_paddock_mesh_statistics
            COMMAND wide_eye --paddock-mesh-statistics-smoke
        )
        add_test(
            NAME wide_eye.opengl_handcrafted_paddock_capture
            COMMAND
                ${CMAKE_COMMAND}
                "-DWIDE_EYE_EXECUTABLE=$<TARGET_FILE:wide_eye>"
                "-DWIDE_EYE_SMOKE_ARGUMENT=--paddock-smoke"
                "-DWIDE_EYE_RESULT_MARKER=paddock_result=pass"
                "-DCAPTURE_ONE=${CMAKE_CURRENT_BINARY_DIR}/paddock-capture-one.png"
                "-DCAPTURE_TWO=${CMAKE_CURRENT_BINARY_DIR}/paddock-capture-two.png"
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
        if(CMAKE_BUILD_TYPE STREQUAL "Release")
            add_test(
                NAME wide_eye.opengl_handcrafted_paddock_performance
                COMMAND wide_eye --paddock-performance-smoke
            )
            add_test(
                NAME wide_eye.opengl_sheep_motion_performance
                COMMAND wide_eye --sheep-motion-performance-smoke
            )
        endif()
    endif()

    set(wide_eye_scaffold_labels headless)
    set(wide_eye_png_writer_labels unit)
    set(wide_eye_voxel_coordinates_labels unit)
    set(wide_eye_chunk_storage_labels unit)
    set(wide_eye_handcrafted_paddock_labels unit)
    set(wide_eye_naive_mesher_labels unit)
    set(wide_eye_chunk_size_comparison_labels performance headless)
    set(wide_eye_core_runtime_labels unit)
    set(wide_eye_window_state_labels unit)
    set(wide_eye_dog_controller_labels unit scenario headless)
    set(wide_eye_gameplay_simulation_labels unit scenario headless)
    set(wide_eye_gameplay_simulation_stack_budget_labels unit headless)
    set(wide_eye_flock_observables_labels unit headless)
    set(wide_eye_sheep_spatial_grid_labels unit headless)
    set(wide_eye_input_labels unit headless)
    set(wide_eye_performance_utilities_labels unit performance headless)
    set(wide_eye_sheep_proxy_labels unit scenario headless)
    set(wide_eye_dog_scenario_labels scenario headless)
    set(wide_eye_ctest_failure_regex_labels unit headless)
    set(wide_eye_core_assertion_labels unit)
    set(wide_eye_window_smoke_labels scenario headless)
    set(wide_eye_accepted_tracer0_baseline_labels scenario headless)
    set(wide_eye_accepted_tracer1_paddock_baseline_labels scenario headless)
    set(wide_eye_context_smoke_labels scenario headless)
    set(wide_eye_triangle_smoke_labels scenario headless)
    set(wide_eye_voxel_cube_smoke_labels scenario headless)
    set(wide_eye_voxel_cube_debug_smoke_labels scenario headless)
    set(wide_eye_voxel_cube_capture_labels scenario headless)
    set(wide_eye_voxel_cube_debug_capture_labels scenario headless)
    set(wide_eye_handcrafted_paddock_render_labels scenario headless)
    set(wide_eye_dog_render_labels scenario headless)
    set(wide_eye_handcrafted_paddock_capture_labels scenario headless)
    set(wide_eye_handcrafted_paddock_debug_labels scenario headless)
    set(wide_eye_gl_debug_labels scenario headless)
    if(WIDE_EYE_ENABLE_SANITIZERS)
        list(APPEND wide_eye_scaffold_labels sanitizer)
        list(APPEND wide_eye_png_writer_labels sanitizer)
        list(APPEND wide_eye_voxel_coordinates_labels sanitizer)
        list(APPEND wide_eye_chunk_storage_labels sanitizer)
        list(APPEND wide_eye_handcrafted_paddock_labels sanitizer)
        list(APPEND wide_eye_naive_mesher_labels sanitizer)
        list(APPEND wide_eye_chunk_size_comparison_labels sanitizer)
        list(APPEND wide_eye_core_runtime_labels sanitizer)
        list(APPEND wide_eye_window_state_labels sanitizer)
        list(APPEND wide_eye_dog_controller_labels sanitizer)
        list(APPEND wide_eye_gameplay_simulation_labels sanitizer)
        list(APPEND wide_eye_gameplay_simulation_stack_budget_labels sanitizer)
        list(APPEND wide_eye_flock_observables_labels sanitizer)
        list(APPEND wide_eye_sheep_spatial_grid_labels sanitizer)
        list(APPEND wide_eye_input_labels sanitizer)
        list(APPEND wide_eye_performance_utilities_labels sanitizer)
        list(APPEND wide_eye_sheep_proxy_labels sanitizer)
        list(APPEND wide_eye_dog_scenario_labels sanitizer)
        list(APPEND wide_eye_core_assertion_labels sanitizer)
        list(APPEND wide_eye_window_smoke_labels sanitizer)
        list(APPEND wide_eye_accepted_tracer0_baseline_labels sanitizer)
        list(APPEND wide_eye_accepted_tracer1_paddock_baseline_labels sanitizer)
        list(APPEND wide_eye_context_smoke_labels sanitizer)
        list(APPEND wide_eye_triangle_smoke_labels sanitizer)
        list(APPEND wide_eye_voxel_cube_smoke_labels sanitizer)
        list(APPEND wide_eye_voxel_cube_debug_smoke_labels sanitizer)
        list(APPEND wide_eye_voxel_cube_capture_labels sanitizer)
        list(APPEND wide_eye_voxel_cube_debug_capture_labels sanitizer)
        list(APPEND wide_eye_handcrafted_paddock_render_labels sanitizer)
        list(APPEND wide_eye_dog_render_labels sanitizer)
        list(APPEND wide_eye_handcrafted_paddock_capture_labels sanitizer)
        list(APPEND wide_eye_handcrafted_paddock_debug_labels sanitizer)
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
        wide_eye.voxel_coordinates
        PROPERTIES LABELS "${wide_eye_voxel_coordinates_labels}"
                   PASS_REGULAR_EXPRESSION "voxel_coordinates_result=pass"
                   TIMEOUT 5
    )
    set_tests_properties(
        wide_eye.chunk_storage
        PROPERTIES LABELS "${wide_eye_chunk_storage_labels}"
                   PASS_REGULAR_EXPRESSION "chunk_storage_result=pass"
                   TIMEOUT 5
    )
    set_tests_properties(
        wide_eye.handcrafted_paddock
        PROPERTIES LABELS "${wide_eye_handcrafted_paddock_labels}"
                   PASS_REGULAR_EXPRESSION "handcrafted_paddock_result=pass"
                   TIMEOUT 5
    )
    set_tests_properties(
        wide_eye.naive_mesher
        PROPERTIES LABELS "${wide_eye_naive_mesher_labels}"
                   PASS_REGULAR_EXPRESSION "naive_mesher_result=pass"
                   TIMEOUT 5
    )
    set_tests_properties(
        wide_eye.chunk_size_comparison
        PROPERTIES LABELS "${wide_eye_chunk_size_comparison_labels}"
                   PASS_REGULAR_EXPRESSION "chunk_size_comparison_result=pass"
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
        wide_eye.dog_controller
        PROPERTIES LABELS "${wide_eye_dog_controller_labels}"
                   PASS_REGULAR_EXPRESSION "dog_controller_result=pass"
                   TIMEOUT 5
    )
    set_tests_properties(
        wide_eye.gameplay_simulation
        PROPERTIES LABELS "${wide_eye_gameplay_simulation_labels}"
                   PASS_REGULAR_EXPRESSION "gameplay_simulation_result=pass"
                   TIMEOUT 5
    )
    if(UNIX)
        set_tests_properties(
            wide_eye.gameplay_simulation_stack_budget
            PROPERTIES LABELS "${wide_eye_gameplay_simulation_stack_budget_labels}"
                       TIMEOUT 30
        )
    endif()
    set_tests_properties(
        wide_eye.flock_observables
        PROPERTIES LABELS "${wide_eye_flock_observables_labels}"
                   PASS_REGULAR_EXPRESSION "flock_observables_result=pass"
                   TIMEOUT 5
    )
    set_tests_properties(
        wide_eye.sheep_spatial_grid
        PROPERTIES LABELS "${wide_eye_sheep_spatial_grid_labels}"
                   PASS_REGULAR_EXPRESSION "sheep_spatial_grid_result=pass"
                   TIMEOUT 5
    )
    set_tests_properties(
        wide_eye.input_actions
        PROPERTIES LABELS "${wide_eye_input_labels}"
                   PASS_REGULAR_EXPRESSION "input_result=pass"
                   TIMEOUT 5
    )
    set_tests_properties(
        wide_eye.performance_utilities
        PROPERTIES LABELS "${wide_eye_performance_utilities_labels}"
                   PASS_REGULAR_EXPRESSION "performance_result=pass"
                   TIMEOUT 5
    )
    set_tests_properties(
        wide_eye.sheep_proxy
        PROPERTIES LABELS "${wide_eye_sheep_proxy_labels}"
                   PASS_REGULAR_EXPRESSION "sheep_proxy_result=pass"
                   TIMEOUT 5
    )
    set_tests_properties(
        wide_eye.dog_wall_scenario
        wide_eye.dog_closed_gate_scenario
        wide_eye.dog_open_gate_scenario
        PROPERTIES LABELS "${wide_eye_dog_scenario_labels}"
                   PASS_REGULAR_EXPRESSION "dog_scenario_result=pass"
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
    set_tests_properties(
        wide_eye.accepted_tracer1_paddock_baseline
        PROPERTIES LABELS "${wide_eye_accepted_tracer1_paddock_baseline_labels}"
                   PASS_REGULAR_EXPRESSION "handcrafted_paddock_baseline_result=pass"
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
            wide_eye.opengl_handcrafted_paddock
            PROPERTIES LABELS "${wide_eye_handcrafted_paddock_render_labels}"
                       PASS_REGULAR_EXPRESSION "paddock_center_matches=yes"
                       TIMEOUT 10
        )
        set_tests_properties(
            wide_eye.opengl_dog_placeholder
            PROPERTIES LABELS "${wide_eye_dog_render_labels}"
                       PASS_REGULAR_EXPRESSION "dog_render_result=pass"
                       TIMEOUT 10
        )
        set_tests_properties(
            wide_eye.opengl_sheep_motion_fixture
            PROPERTIES LABELS "${wide_eye_dog_render_labels}"
                       PASS_REGULAR_EXPRESSION "presentation_motion_fixture=scripted_non_behavior"
                       TIMEOUT 10
        )
        set_tests_properties(
            wide_eye.opengl_handcrafted_paddock_capture
            PROPERTIES LABELS "${wide_eye_handcrafted_paddock_capture_labels}"
                       PASS_REGULAR_EXPRESSION "capture_repeat_sha256="
                       TIMEOUT 20
        )
        set_tests_properties(
            wide_eye.opengl_handcrafted_paddock_chunk_bounds
            wide_eye.opengl_handcrafted_paddock_face_normals
            wide_eye.opengl_handcrafted_paddock_wireframe
            wide_eye.opengl_handcrafted_paddock_mesh_statistics
            PROPERTIES LABELS "${wide_eye_handcrafted_paddock_debug_labels}"
                       PASS_REGULAR_EXPRESSION "paddock_debug_frame_matches=yes"
                       TIMEOUT 10
        )
        set_tests_properties(
            wide_eye.opengl_debug_high_severity
            PROPERTIES LABELS "${wide_eye_gl_debug_labels}"
                       TIMEOUT 5
        )
        if(CMAKE_BUILD_TYPE STREQUAL "Release")
            set_tests_properties(
                wide_eye.opengl_handcrafted_paddock_performance
                wide_eye.opengl_sheep_motion_performance
                PROPERTIES LABELS "performance;scenario;headless"
                           PASS_REGULAR_EXPRESSION "within_provisional_low_budget=yes"
                           TIMEOUT 60
            )
        endif()
    endif()

    set(wide_eye_registered_tests
        wide_eye.scaffold_version
        wide_eye.png_writer
        wide_eye.voxel_coordinates
        wide_eye.chunk_storage
        wide_eye.handcrafted_paddock
        wide_eye.naive_mesher
        wide_eye.chunk_size_comparison
        wide_eye.core_runtime
        wide_eye.window_state
        wide_eye.dog_controller
        wide_eye.gameplay_simulation
        wide_eye.flock_observables
        wide_eye.sheep_spatial_grid
        wide_eye.input_actions
        wide_eye.performance_utilities
        wide_eye.sheep_proxy
        wide_eye.dog_wall_scenario
        wide_eye.dog_closed_gate_scenario
        wide_eye.dog_open_gate_scenario
        wide_eye.ctest_failure_regex
        wide_eye.core_assertion
        wide_eye.sdl_window_smoke
        wide_eye.accepted_tracer0_baseline
        wide_eye.accepted_tracer1_paddock_baseline
    )
    if(UNIX)
        list(APPEND wide_eye_registered_tests wide_eye.gameplay_simulation_stack_budget)
    endif()
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
            wide_eye.opengl_handcrafted_paddock
            wide_eye.opengl_dog_placeholder
            wide_eye.opengl_sheep_motion_fixture
            wide_eye.opengl_handcrafted_paddock_capture
            wide_eye.opengl_handcrafted_paddock_chunk_bounds
            wide_eye.opengl_handcrafted_paddock_face_normals
            wide_eye.opengl_handcrafted_paddock_wireframe
            wide_eye.opengl_handcrafted_paddock_mesh_statistics
            wide_eye.opengl_debug_high_severity
        )
        if(CMAKE_BUILD_TYPE STREQUAL "Release")
            list(
                APPEND
                wide_eye_registered_tests
                wide_eye.opengl_handcrafted_paddock_performance
                wide_eye.opengl_sheep_motion_performance
            )
        endif()
    endif()
    set_tests_properties(
        ${wide_eye_registered_tests}
        PROPERTIES FAIL_REGULAR_EXPRESSION
                   "failure_stage=|ERROR: AddressSanitizer|ERROR: LeakSanitizer|runtime error:"
    )
endif()
