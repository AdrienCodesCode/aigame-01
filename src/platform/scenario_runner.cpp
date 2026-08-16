#include "platform/scenario_runner.hpp"

#include "game/camera_controller.hpp"
#include "game/gameplay_replay.hpp"
#include "game/gameplay_simulation.hpp"
#include "platform/window_runtime.hpp"
#include "render/opengl_renderer.hpp"
#include "render/png_writer.hpp"
#include "voxel/handcrafted_paddock.hpp"

#include <array>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <string_view>
#include <utility>

namespace wide_eye::platform {
namespace {

constexpr int kWindowWidth = 960;
constexpr int kWindowHeight = 540;
constexpr int kContextSmokeSize = 64;

enum class RenderScenario {
    interactive_paddock,
    dog_paddock,
    sheep_motion_paddock,
    sheep_motion_debug_paddock,
    sheep_motion_performance_paddock,
    handcrafted_paddock_performance,
    triangle,
    voxel_cube,
    voxel_cube_debug,
    handcrafted_paddock,
    handcrafted_paddock_chunk_bounds,
    handcrafted_paddock_face_normals,
    handcrafted_paddock_wireframe,
    handcrafted_paddock_mesh_statistics,
};

[[nodiscard]] bool is_handcrafted_paddock_scenario(RenderScenario scenario) {
    return scenario == RenderScenario::interactive_paddock ||
           scenario == RenderScenario::dog_paddock ||
           scenario == RenderScenario::sheep_motion_paddock ||
           scenario == RenderScenario::sheep_motion_debug_paddock ||
           scenario == RenderScenario::sheep_motion_performance_paddock ||
           scenario == RenderScenario::handcrafted_paddock_performance ||
           scenario == RenderScenario::handcrafted_paddock ||
           scenario == RenderScenario::handcrafted_paddock_chunk_bounds ||
           scenario == RenderScenario::handcrafted_paddock_face_normals ||
           scenario == RenderScenario::handcrafted_paddock_wireframe ||
           scenario == RenderScenario::handcrafted_paddock_mesh_statistics;
}

[[nodiscard]] bool has_gameplay_dog(RenderScenario scenario) {
    return scenario == RenderScenario::interactive_paddock ||
           scenario == RenderScenario::dog_paddock ||
           scenario == RenderScenario::sheep_motion_paddock ||
           scenario == RenderScenario::sheep_motion_debug_paddock ||
           scenario == RenderScenario::sheep_motion_performance_paddock;
}

[[nodiscard]] render::HandcraftedPaddockView paddock_view(RenderScenario scenario) {
    switch (scenario) {
    case RenderScenario::handcrafted_paddock_chunk_bounds:
        return render::HandcraftedPaddockView::chunk_bounds;
    case RenderScenario::handcrafted_paddock_face_normals:
        return render::HandcraftedPaddockView::face_normals;
    case RenderScenario::handcrafted_paddock_wireframe:
        return render::HandcraftedPaddockView::wireframe;
    case RenderScenario::handcrafted_paddock_mesh_statistics:
        return render::HandcraftedPaddockView::mesh_statistics;
    case RenderScenario::interactive_paddock:
    case RenderScenario::dog_paddock:
    case RenderScenario::sheep_motion_paddock:
    case RenderScenario::sheep_motion_performance_paddock:
    case RenderScenario::handcrafted_paddock_performance:
    case RenderScenario::handcrafted_paddock:
    case RenderScenario::triangle:
    case RenderScenario::voxel_cube:
    case RenderScenario::voxel_cube_debug:
        return render::HandcraftedPaddockView::normal;
    case RenderScenario::sheep_motion_debug_paddock:
        return render::HandcraftedPaddockView::face_normals;
    }
    return render::HandcraftedPaddockView::normal;
}

[[nodiscard]] std::string_view paddock_view_name(render::HandcraftedPaddockView view) {
    switch (view) {
    case render::HandcraftedPaddockView::normal:
        return "normal";
    case render::HandcraftedPaddockView::chunk_bounds:
        return "chunk_bounds";
    case render::HandcraftedPaddockView::face_normals:
        return "face_normals";
    case render::HandcraftedPaddockView::wireframe:
        return "wireframe";
    case render::HandcraftedPaddockView::mesh_statistics:
        return "mesh_statistics";
    }
    return "unknown";
}

class NoOpScenarioRunner final : public WindowScenarioRunner {
  public:
    WindowResult render_frame(const WindowState&, double) override {
        return std::nullopt;
    }
};

class RenderScenarioRunner final : public WindowScenarioRunner {
  public:
    RenderScenarioRunner(
        RenderScenario scenario, std::optional<std::filesystem::path> capture_path = std::nullopt,
        std::optional<game::GameplayScenarioDefinition> gameplay_scenario = std::nullopt,
        std::uint64_t capture_tick = 61,
        std::optional<std::filesystem::path> state_dump_path = std::nullopt)
        : scenario_{scenario}, capture_path_{std::move(capture_path)},
          gameplay_scenario_{gameplay_scenario}, capture_tick_{capture_tick},
          state_dump_path_{std::move(state_dump_path)} {}

    WindowResult initialize() override {
        renderer_.emplace();
        if (!renderer_->initialize(std::cerr)) {
            return WindowFailure{"renderer_init", false};
        }

        if (is_handcrafted_paddock_scenario(scenario_)) {
            const auto paddock = voxel::make_handcrafted_paddock();
            if (!paddock.has_value()) {
                return WindowFailure{"paddock_scene_build", false};
            }
            const auto mesh = voxel::build_handcrafted_paddock_mesh(*paddock);
            if (!mesh.has_value()) {
                return WindowFailure{"paddock_mesh_build", false};
            }
            if (!mesh->passes.cutout.vertices.empty() || !mesh->passes.cutout.indices.empty() ||
                !mesh->passes.translucent.vertices.empty() ||
                !mesh->passes.translucent.indices.empty()) {
                return WindowFailure{"paddock_mesh_pass", false};
            }
            const voxel::PaddockPalette palette = voxel::make_handcrafted_paddock_palette();
            if (!renderer_->upload_handcrafted_paddock(mesh->passes.opaque, palette,
                                                       mesh->source_chunk_count,
                                                       mesh->occupied_block_count, std::cerr)) {
                return WindowFailure{"paddock_mesh_upload", false};
            }
            paddock_chunk_count_ = mesh->source_chunk_count;
            paddock_occupied_block_count_ = mesh->occupied_block_count;
            paddock_face_count_ = mesh->passes.opaque.face_count();
            paddock_vertex_count_ = mesh->passes.opaque.vertices.size();
            paddock_index_count_ = mesh->passes.opaque.indices.size();
            paddock_faces_by_material_ = mesh->opaque_faces_by_material;
            paddock_face_decision_count_ = mesh->face_diagnostics.size();
            for (const voxel::HandcraftedPaddockMesh::FaceDiagnostic& diagnostic :
                 mesh->face_diagnostics) {
                if (diagnostic.face.disposition == voxel::FaceDisposition::emitted) {
                    ++paddock_emitted_face_decision_count_;
                } else {
                    ++paddock_culled_face_decision_count_;
                }
                ++paddock_face_decisions_by_neighbor_kind_[voxel::face_neighbor_kind_index(
                    diagnostic.face.neighbor_kind)];
            }
            if (paddock_face_decision_count_ !=
                    paddock_occupied_block_count_ * voxel::kFaceDirectionCount ||
                paddock_emitted_face_decision_count_ != paddock_face_count_ ||
                paddock_emitted_face_decision_count_ + paddock_culled_face_decision_count_ !=
                    paddock_face_decision_count_) {
                return WindowFailure{"paddock_face_diagnostics", false};
            }

            if (has_gameplay_dog(scenario_)) {
                if (!gameplay_scenario_.has_value()) {
                    gameplay_scenario_ = game::find_gameplay_scenario("paddock-start");
                }
                if (!gameplay_scenario_.has_value()) {
                    return WindowFailure{"dog_scenario_select", false};
                }
                simulation_.emplace(*gameplay_scenario_);
                camera_.emplace(simulation_->current_snapshot().dog);
                previous_camera_state_ = camera_->state();
                if (scenario_ == RenderScenario::sheep_motion_paddock ||
                    scenario_ == RenderScenario::sheep_motion_debug_paddock ||
                    scenario_ == RenderScenario::sheep_motion_performance_paddock) {
                    for (std::uint64_t tick = 0; tick < capture_tick_; ++tick) {
                        simulation_->fixed_update({});
                    }
                    std::cout << "presentation_motion_fixture=scripted_non_behavior\n"
                              << "presentation_motion_tick=" << capture_tick_ << '\n'
                              << "presentation_motion_interpolation_alpha=0.5\n";
                    if (state_dump_path_.has_value()) {
                        const game::GameplayTextResult state =
                            game::gameplay_state_dump_json(*simulation_);
                        if (!state) {
                            return WindowFailure{"state_dump_encode", false};
                        }
                        std::ofstream output{*state_dump_path_, std::ios::binary | std::ios::trunc};
                        output << state.text << '\n';
                        if (!output) {
                            return WindowFailure{"state_dump_write", false};
                        }
                        std::cout << "state_dump_path=" << state_dump_path_->string() << '\n'
                                  << "state_dump_schema=wide-eye.gameplay-state\n"
                                  << "state_dump_version=" << game::kGameplayStateDumpFormatVersion
                                  << '\n';
                    }
                }
                std::cout << "gameplay_scenario="
                          << game::gameplay_scenario_name(gameplay_scenario_->id) << '\n'
                          << "gameplay_scenario_version=" << gameplay_scenario_->version << '\n'
                          << "gameplay_scenario_seed=" << gameplay_scenario_->seed << '\n'
                          << "dog_collision=analytic_upright_cylinder\n"
                          << "input_actions=named\n";
            }
        }
        return std::nullopt;
    }

    WindowResult prepare_performance_frame(double interpolation_alpha) override {
        if (scenario_ != RenderScenario::sheep_motion_performance_paddock) {
            return std::nullopt;
        }
        prepared_gameplay_frame_ = make_gameplay_frame(interpolation_alpha);
        return std::nullopt;
    }

    WindowResult fixed_update(const NamedActionSnapshot& input,
                              double fixed_delta_seconds) override {
        if (!simulation_.has_value() || !camera_.has_value()) {
            return std::nullopt;
        }

        if (input.action(NamedAction::restart).pressed) {
            simulation_->restart();
            camera_->restart(simulation_->current_snapshot().dog);
            previous_camera_state_ = camera_->state();
            std::cout << "dog_restart_count=" << simulation_->restart_count() << '\n';
            return std::nullopt;
        }

        previous_camera_state_ = camera_->state();
        const bool toggle_camera = input.action(NamedAction::toggle_camera).pressed;
        const bool debug_controls =
            camera_->mode() == game::CameraMode::free_debug || toggle_camera;
        const double move_right =
            static_cast<double>(input.signed_axis(NamedAction::move_left, NamedAction::move_right));
        const double move_forward = static_cast<double>(
            input.signed_axis(NamedAction::move_backward, NamedAction::move_forward));
        const MouseLookDelta mouse_look = input.mouse_look_delta();
        camera_->fixed_update(
            simulation_->current_snapshot().dog,
            {
                .move_right = debug_controls ? move_right : 0.0,
                .move_forward = debug_controls ? move_forward : 0.0,
                .rise = static_cast<double>(
                    input.signed_axis(NamedAction::camera_fall, NamedAction::camera_rise)),
                .look_right_rate = static_cast<double>(input.signed_axis(
                    NamedAction::camera_look_left, NamedAction::camera_look_right)),
                .look_up_rate = static_cast<double>(
                    input.signed_axis(NamedAction::camera_look_down, NamedAction::camera_look_up)),
                .look_right_delta = static_cast<double>(mouse_look.right),
                .look_up_delta = static_cast<double>(mouse_look.up),
                .toggle_mode = toggle_camera,
            },
            fixed_delta_seconds);

        if (toggle_camera) {
            simulation_->fixed_update({});
            previous_camera_state_ = camera_->state();
            return std::nullopt;
        }
        if (camera_->mode() == game::CameraMode::gameplay) {
            const game::Vec3 desired_move = game::resolve_camera_relative_move(
                camera_->state().gameplay_yaw, move_right, move_forward);
            simulation_->fixed_update({.dog_move = game::DogMoveInput{
                                           .world_x = desired_move.x,
                                           .world_z = desired_move.z,
                                           .sprint = input.action(NamedAction::sprint).value > 0.5F,
                                       }});
        } else {
            simulation_->fixed_update({});
        }
        return std::nullopt;
    }

    WindowResult render_frame(const WindowState& window_state,
                              double interpolation_alpha) override {
        if (scenario_ == RenderScenario::interactive_paddock) {
            render_gameplay_paddock(window_state, interpolation_alpha);
            return std::nullopt;
        }

        if (scenario_ == RenderScenario::triangle) {
            return render_triangle(window_state);
        }
        if (scenario_ == RenderScenario::sheep_motion_performance_paddock) {
            render_gameplay_paddock(window_state, interpolation_alpha);
            return std::nullopt;
        }
        if (scenario_ == RenderScenario::handcrafted_paddock_performance) {
            renderer_->render_handcrafted_paddock(window_state.pixel_width(),
                                                  window_state.pixel_height());
            return std::nullopt;
        }
        if (is_handcrafted_paddock_scenario(scenario_)) {
            return render_handcrafted_paddock(window_state, interpolation_alpha);
        }

        return render_voxel_cube(window_state);
    }

    void release_graphics_resources() override {
        renderer_.reset();
    }

  private:
    [[nodiscard]] render::HandcraftedPaddockFrame
    make_gameplay_frame(double interpolation_alpha) const {
        if (scenario_ == RenderScenario::sheep_motion_paddock ||
            scenario_ == RenderScenario::sheep_motion_debug_paddock ||
            scenario_ == RenderScenario::sheep_motion_performance_paddock) {
            interpolation_alpha = 0.5;
        }
        const game::GameplaySnapshot render_snapshot =
            simulation_->interpolated_snapshot(interpolation_alpha);
        const game::DogState& render_dog = render_snapshot.dog;
        const render::SheepProxyPoseBuffer render_sheep =
            render::make_sheep_proxy_poses(render_snapshot);
        const game::CameraState render_camera = game::interpolate_camera_state(
            previous_camera_state_, camera_->state(), interpolation_alpha);
        const game::CameraPose game_camera = game::camera_pose(render_dog, render_camera);
        return {
            .view = paddock_view(scenario_),
            .camera =
                {
                    .eye = {static_cast<float>(game_camera.eye.x),
                            static_cast<float>(game_camera.eye.y),
                            static_cast<float>(game_camera.eye.z)},
                    .target = {static_cast<float>(game_camera.target.x),
                               static_cast<float>(game_camera.target.y),
                               static_cast<float>(game_camera.target.z)},
                },
            .dog =
                render::DogRenderPose{
                    .ground_position = {static_cast<float>(render_dog.position.x),
                                        static_cast<float>(render_dog.position.y),
                                        static_cast<float>(render_dog.position.z)},
                    .heading_radians = static_cast<float>(render_dog.heading_radians),
                },
            .sheep = render_sheep,
            .sheep_count = render_sheep.size(),
        };
    }

    void render_gameplay_paddock(const WindowState& window_state, double interpolation_alpha) {
        const render::HandcraftedPaddockFrame frame =
            prepared_gameplay_frame_.has_value() ? *prepared_gameplay_frame_
                                                 : make_gameplay_frame(interpolation_alpha);
        prepared_gameplay_frame_.reset();
        renderer_->render_handcrafted_paddock(window_state.pixel_width(),
                                              window_state.pixel_height(), frame);
    }

    WindowResult render_triangle(const WindowState& window_state) {
        renderer_->render_triangle(window_state.pixel_width(), window_state.pixel_height());
        std::cout << "triangle_draw=issued\n";
        const render::TriangleSample sample = renderer_->sample_triangle_center(
            window_state.pixel_width(), window_state.pixel_height());
        const bool sample_matches = render::is_expected_triangle_sample(sample);
        std::cout << "triangle_center_rgba=" << static_cast<unsigned int>(sample.red) << ','
                  << static_cast<unsigned int>(sample.green) << ','
                  << static_cast<unsigned int>(sample.blue) << ','
                  << static_cast<unsigned int>(sample.alpha) << '\n'
                  << "triangle_center_matches=" << (sample_matches ? "yes" : "no") << '\n';
        if (!sample_matches) {
            return WindowFailure{"triangle_framebuffer_oracle", false};
        }
        return std::nullopt;
    }

    WindowResult render_voxel_cube(const WindowState& window_state) {
        const bool debug = scenario_ == RenderScenario::voxel_cube_debug;
        if (debug) {
            renderer_->render_voxel_cube_wireframe(window_state.pixel_width(),
                                                   window_state.pixel_height());
        } else {
            renderer_->render_voxel_cube(window_state.pixel_width(), window_state.pixel_height());
        }
        std::cout << "scenario=voxel_cube_smoke\n"
                  << "voxel_cube_view=" << (debug ? "wireframe_debug" : "normal") << '\n'
                  << "voxel_cube_draw=issued\n";
        const render::VoxelCubeSample sample = renderer_->sample_voxel_cube_center(
            window_state.pixel_width(), window_state.pixel_height());
        const bool depth_state_matches =
            sample.depth_test_enabled && sample.depth_function_less && sample.depth_write_enabled;
        const bool sample_matches =
            debug ? depth_state_matches : render::is_expected_voxel_cube_sample(sample);
        std::cout << "voxel_cube_center_rgba=" << static_cast<unsigned int>(sample.color.red) << ','
                  << static_cast<unsigned int>(sample.color.green) << ','
                  << static_cast<unsigned int>(sample.color.blue) << ','
                  << static_cast<unsigned int>(sample.color.alpha) << '\n'
                  << "voxel_cube_center_depth=" << sample.depth << '\n'
                  << "depth_test_enabled=" << (sample.depth_test_enabled ? "yes" : "no") << '\n'
                  << "depth_function_less=" << (sample.depth_function_less ? "yes" : "no") << '\n'
                  << "depth_write_enabled=" << (sample.depth_write_enabled ? "yes" : "no") << '\n';
        if (debug) {
            std::cout << "voxel_cube_depth_state_matches=" << (depth_state_matches ? "yes" : "no")
                      << '\n';
        } else {
            std::cout << "voxel_cube_center_matches=" << (sample_matches ? "yes" : "no") << '\n';
        }
        if (!sample_matches) {
            return WindowFailure{debug ? "voxel_cube_debug_depth_state" : "voxel_cube_depth_oracle",
                                 false};
        }

        if (capture_path_.has_value() || debug) {
            const std::optional<render::Rgba8Frame> frame = renderer_->capture_rgba8(
                window_state.pixel_width(), window_state.pixel_height(), std::cerr);
            if (!frame.has_value()) {
                return WindowFailure{"capture_readback", false};
            }
            if (debug) {
                const std::size_t visible_pixels =
                    render::count_voxel_cube_wireframe_pixels(*frame);
                const bool debug_frame_matches = render::is_expected_voxel_cube_wireframe(*frame);
                std::cout << "wireframe_visible_pixels=" << visible_pixels << '\n'
                          << "wireframe_frame_matches=" << (debug_frame_matches ? "yes" : "no")
                          << '\n';
                if (!debug_frame_matches) {
                    return WindowFailure{"voxel_cube_wireframe_oracle", false};
                }
            }
            if (capture_path_.has_value()) {
                if (!render::write_png_rgba8(*capture_path_, frame->width, frame->height,
                                             frame->pixels, std::cerr)) {
                    return WindowFailure{"capture_write", false};
                }
                std::cout << "capture_path=" << capture_path_->string() << '\n'
                          << "capture_width=" << frame->width << '\n'
                          << "capture_height=" << frame->height << '\n'
                          << "capture_format=png_rgba8\n"
                          << "capture_result=pass\n";
            }
        }

        return std::nullopt;
    }

    WindowResult render_handcrafted_paddock(const WindowState& window_state,
                                            double interpolation_alpha) {
        const render::HandcraftedPaddockView view = paddock_view(scenario_);
        if (has_gameplay_dog(scenario_)) {
            render_gameplay_paddock(window_state, interpolation_alpha);
        } else {
            renderer_->render_handcrafted_paddock(
                window_state.pixel_width(), window_state.pixel_height(),
                {.view = view, .camera = {}, .dog = std::nullopt});
        }
        std::cout << "scenario=handcrafted_paddock\n"
                  << "paddock_view=" << paddock_view_name(view) << '\n'
                  << "paddock_chunks=" << paddock_chunk_count_ << '\n'
                  << "paddock_occupied_blocks=" << paddock_occupied_block_count_ << '\n'
                  << "paddock_faces=" << paddock_face_count_ << '\n'
                  << "paddock_vertices=" << paddock_vertex_count_ << '\n'
                  << "paddock_indices=" << paddock_index_count_ << '\n'
                  << "paddock_face_decisions=" << paddock_face_decision_count_ << '\n'
                  << "paddock_emitted_face_decisions=" << paddock_emitted_face_decision_count_
                  << '\n'
                  << "paddock_culled_face_decisions=" << paddock_culled_face_decision_count_ << '\n'
                  << "paddock_same_chunk_neighbor_decisions="
                  << paddock_face_decisions_by_neighbor_kind_[voxel::face_neighbor_kind_index(
                         voxel::FaceNeighborKind::same_chunk)]
                  << '\n'
                  << "paddock_adjacent_chunk_neighbor_decisions="
                  << paddock_face_decisions_by_neighbor_kind_[voxel::face_neighbor_kind_index(
                         voxel::FaceNeighborKind::adjacent_chunk)]
                  << '\n'
                  << "paddock_missing_chunk_neighbor_decisions="
                  << paddock_face_decisions_by_neighbor_kind_[voxel::face_neighbor_kind_index(
                         voxel::FaceNeighborKind::missing_chunk)]
                  << '\n'
                  << "paddock_grass_faces="
                  << paddock_faces_by_material_[voxel::kPaddockGrassMaterial.value] << '\n'
                  << "paddock_stone_faces="
                  << paddock_faces_by_material_[voxel::kPaddockStoneMaterial.value] << '\n'
                  << "paddock_gate_faces="
                  << paddock_faces_by_material_[voxel::kPaddockGateMaterial.value] << '\n'
                  << "paddock_barn_wall_faces="
                  << paddock_faces_by_material_[voxel::kPaddockBarnWallMaterial.value] << '\n'
                  << "paddock_barn_roof_faces="
                  << paddock_faces_by_material_[voxel::kPaddockBarnRoofMaterial.value] << '\n'
                  << "paddock_barn_door_faces="
                  << paddock_faces_by_material_[voxel::kPaddockBarnDoorMaterial.value] << '\n'
                  << "paddock_draw=issued\n";
        if (view == render::HandcraftedPaddockView::mesh_statistics) {
            std::cout << "mesh_stats_overlay_order=chunks,occupied_blocks,faces,vertices,indices\n";
        }

        const render::VoxelCubeSample sample = renderer_->sample_handcrafted_paddock_center(
            window_state.pixel_width(), window_state.pixel_height());
        const bool depth_state_matches =
            sample.depth_test_enabled && sample.depth_function_less && sample.depth_write_enabled;
        const bool sample_matches =
            view == render::HandcraftedPaddockView::normal && !has_gameplay_dog(scenario_)
                ? render::is_expected_handcrafted_paddock_sample(sample)
                : depth_state_matches;
        std::cout << "paddock_center_rgba=" << static_cast<unsigned int>(sample.color.red) << ','
                  << static_cast<unsigned int>(sample.color.green) << ','
                  << static_cast<unsigned int>(sample.color.blue) << ','
                  << static_cast<unsigned int>(sample.color.alpha) << '\n'
                  << "paddock_center_depth=" << sample.depth << '\n'
                  << "depth_test_enabled=" << (sample.depth_test_enabled ? "yes" : "no") << '\n'
                  << "depth_function_less=" << (sample.depth_function_less ? "yes" : "no") << '\n'
                  << "depth_write_enabled=" << (sample.depth_write_enabled ? "yes" : "no") << '\n'
                  << "paddock_center_matches=" << (sample_matches ? "yes" : "no") << '\n';
        if (!sample_matches) {
            return WindowFailure{view == render::HandcraftedPaddockView::normal &&
                                         !has_gameplay_dog(scenario_)
                                     ? "paddock_framebuffer_oracle"
                                     : "paddock_debug_depth_state",
                                 false};
        }

        if (capture_path_.has_value() || view != render::HandcraftedPaddockView::normal) {
            const std::optional<render::Rgba8Frame> frame = renderer_->capture_rgba8(
                window_state.pixel_width(), window_state.pixel_height(), std::cerr);
            if (!frame.has_value()) {
                return WindowFailure{"capture_readback", false};
            }
            if (view != render::HandcraftedPaddockView::normal) {
                const std::size_t debug_pixels =
                    render::count_handcrafted_paddock_debug_pixels(*frame, view);
                const bool debug_frame_matches =
                    render::is_expected_handcrafted_paddock_debug_frame(*frame, view);
                std::cout << "paddock_debug_pixels=" << debug_pixels << '\n'
                          << "paddock_debug_frame_matches=" << (debug_frame_matches ? "yes" : "no")
                          << '\n';
                if (!debug_frame_matches) {
                    return WindowFailure{"paddock_debug_frame_oracle", false};
                }
            }
            if (capture_path_.has_value()) {
                if (!render::write_png_rgba8(*capture_path_, frame->width, frame->height,
                                             frame->pixels, std::cerr)) {
                    return WindowFailure{"capture_write", false};
                }
                std::cout << "capture_path=" << capture_path_->string() << '\n'
                          << "capture_width=" << frame->width << '\n'
                          << "capture_height=" << frame->height << '\n'
                          << "capture_format=png_rgba8\n"
                          << "capture_result=pass\n";
            }
        }

        return std::nullopt;
    }

    RenderScenario scenario_;
    std::optional<std::filesystem::path> capture_path_;
    std::optional<game::GameplayScenarioDefinition> gameplay_scenario_;
    std::uint64_t capture_tick_ = 61;
    std::optional<std::filesystem::path> state_dump_path_;
    std::optional<render::OpenGlRenderer> renderer_;
    std::optional<game::GameplaySimulation> simulation_;
    std::optional<game::CameraController> camera_;
    game::CameraState previous_camera_state_{};
    std::optional<render::HandcraftedPaddockFrame> prepared_gameplay_frame_;
    std::size_t paddock_chunk_count_ = 0;
    std::size_t paddock_occupied_block_count_ = 0;
    std::size_t paddock_face_count_ = 0;
    std::size_t paddock_vertex_count_ = 0;
    std::size_t paddock_index_count_ = 0;
    std::array<std::size_t, voxel::PaddockPalette::kEntryCount> paddock_faces_by_material_{};
    std::size_t paddock_face_decision_count_ = 0;
    std::size_t paddock_emitted_face_decision_count_ = 0;
    std::size_t paddock_culled_face_decision_count_ = 0;
    std::array<std::size_t, voxel::kFaceNeighborKindCount>
        paddock_face_decisions_by_neighbor_kind_{};
};

WindowRunConfiguration interactive_configuration() {
    WindowRunConfiguration configuration{};
    configuration.result_name = "context_result";
    configuration.width = kWindowWidth;
    configuration.height = kWindowHeight;
    configuration.use_opengl = true;
    configuration.bounded = false;
    configuration.resizable = true;
    configuration.require_depth_buffer = true;
    configuration.request_vsync = true;
    configuration.render_interactive_frames = true;
    return configuration;
}

WindowRunConfiguration bounded_configuration(std::string_view result_name, bool use_opengl) {
    WindowRunConfiguration configuration{};
    configuration.result_name = result_name;
    configuration.width = use_opengl ? kContextSmokeSize : kWindowWidth;
    configuration.height = use_opengl ? kContextSmokeSize : kWindowHeight;
    configuration.use_opengl = use_opengl;
    configuration.hidden = use_opengl;
    return configuration;
}

} // namespace

int run_interactive_scenario(std::string_view dog_scenario) {
    const auto definition = game::find_gameplay_scenario(dog_scenario);
    if (!definition.has_value()) {
        std::cerr << "dog_scenario_result=fail\n"
                  << "failure_stage=dog_scenario_select\n";
        return EXIT_FAILURE;
    }
    RenderScenarioRunner runner{RenderScenario::interactive_paddock, std::nullopt, definition};
    WindowRunConfiguration configuration = interactive_configuration();
    configuration.enable_input = true;
    return run_window(configuration, runner);
}

int run_dog_headless_scenario(std::string_view dog_scenario) {
    const auto definition = game::find_gameplay_scenario(dog_scenario);
    if (!definition.has_value()) {
        std::cerr << "dog_scenario_result=fail\n"
                  << "failure_stage=dog_scenario_select\n";
        return EXIT_FAILURE;
    }
    game::GameplaySimulation simulation{*definition};
    for (int tick = 0; tick < 240; ++tick) {
        simulation.fixed_update({.dog_move = game::DogMoveInput{.world_z = -1.0}});
    }
    const game::DogState final_state = simulation.current_snapshot().dog;
    const bool expected_contact = definition->id == game::GameplayScenarioId::wall_contact ||
                                  definition->id == game::GameplayScenarioId::closed_gate;
    const bool expected_passage = definition->id == game::GameplayScenarioId::open_gate;
    const bool result_matches =
        final_state.grounded &&
        (!expected_contact || final_state.position.z >= 16.0 + game::DogController::kRadius) &&
        (!expected_passage || final_state.position.z < 14.0);
    simulation.restart();
    const bool restart_matches =
        simulation.current_snapshot().dog == definition->dog.initial_state &&
        simulation.current_snapshot().tick == 0;
    std::cout << "dog_scenario=" << game::gameplay_scenario_name(definition->id) << '\n'
              << "dog_scenario_version=" << definition->version << '\n'
              << "dog_scenario_seed=" << definition->seed << '\n'
              << "dog_ticks=240\n"
              << "dog_final_x=" << final_state.position.x << '\n'
              << "dog_final_y=" << final_state.position.y << '\n'
              << "dog_final_z=" << final_state.position.z << '\n'
              << "dog_grounded=" << (final_state.grounded ? "yes" : "no") << '\n'
              << "dog_restart_matches=" << (restart_matches ? "yes" : "no") << '\n'
              << "dog_scenario_result=" << (result_matches && restart_matches ? "pass" : "fail")
              << '\n';
    return result_matches && restart_matches ? EXIT_SUCCESS : EXIT_FAILURE;
}

int run_dog_render_scenario(std::string_view dog_scenario,
                            const std::optional<std::filesystem::path>& capture_path) {
    const auto definition = game::find_gameplay_scenario(dog_scenario);
    if (!definition.has_value()) {
        std::cerr << "dog_render_result=fail\n"
                  << "failure_stage=dog_scenario_select\n";
        return EXIT_FAILURE;
    }
    RenderScenarioRunner runner{RenderScenario::dog_paddock, capture_path, definition};
    WindowRunConfiguration configuration = bounded_configuration("dog_render_result", true);
    configuration.width = kWindowWidth;
    configuration.height = kWindowHeight;
    configuration.require_depth_buffer = true;
    configuration.render_bounded_frame = true;
    return run_window(configuration, runner);
}

int run_sheep_motion_render_scenario(const std::optional<std::filesystem::path>& capture_path,
                                     std::uint64_t capture_tick, bool debug_view,
                                     const std::optional<std::filesystem::path>& state_dump_path) {
    const auto definition = game::find_gameplay_scenario("presentation-motion");
    if (!definition.has_value()) {
        std::cerr << "sheep_motion_render_result=fail\n"
                  << "failure_stage=sheep_motion_scenario_select\n";
        return EXIT_FAILURE;
    }
    RenderScenarioRunner runner{debug_view ? RenderScenario::sheep_motion_debug_paddock
                                           : RenderScenario::sheep_motion_paddock,
                                capture_path, definition, capture_tick, state_dump_path};
    WindowRunConfiguration configuration =
        bounded_configuration("sheep_motion_render_result", true);
    configuration.width = 1920;
    configuration.height = 1080;
    configuration.require_depth_buffer = true;
    configuration.render_bounded_frame = true;
    return run_window(configuration, runner);
}

int run_sheep_motion_performance_scenario() {
    const auto definition = game::find_gameplay_scenario("presentation-motion");
    if (!definition.has_value()) {
        std::cerr << "sheep_motion_performance_result=fail\n"
                  << "failure_stage=sheep_motion_scenario_select\n";
        return EXIT_FAILURE;
    }
    RenderScenarioRunner runner{RenderScenario::sheep_motion_performance_paddock, std::nullopt,
                                definition, 61};
    WindowRunConfiguration configuration =
        bounded_configuration("sheep_motion_performance_result", true);
    configuration.width = 1920;
    configuration.height = 1080;
    configuration.require_depth_buffer = true;
    configuration.performance_warmup_frames = 120;
    configuration.performance_sample_frames = 600;
    configuration.performance_scenario = "presentation_motion_five_proxy_v1";
    configuration.performance_budget = core::kTracer2LowProfilePerformanceBudget;
    return run_window(configuration, runner);
}

int run_window_smoke_scenario() {
    NoOpScenarioRunner runner;
    WindowRunConfiguration configuration = bounded_configuration("window_result", false);
    configuration.validate_window_events = true;
    return run_window(configuration, runner);
}

int run_context_smoke_scenario() {
    NoOpScenarioRunner runner;
    return run_window(bounded_configuration("context_result", true), runner);
}

int run_triangle_smoke_scenario() {
    RenderScenarioRunner runner{RenderScenario::triangle};
    WindowRunConfiguration configuration = bounded_configuration("triangle_result", true);
    configuration.render_bounded_frame = true;
    return run_window(configuration, runner);
}

int run_voxel_cube_smoke_scenario(const std::optional<std::filesystem::path>& capture_path) {
    RenderScenarioRunner runner{RenderScenario::voxel_cube, capture_path};
    WindowRunConfiguration configuration = bounded_configuration("voxel_cube_result", true);
    configuration.require_depth_buffer = true;
    configuration.render_bounded_frame = true;
    return run_window(configuration, runner);
}

int run_voxel_cube_debug_smoke_scenario(const std::optional<std::filesystem::path>& capture_path) {
    RenderScenarioRunner runner{RenderScenario::voxel_cube_debug, capture_path};
    WindowRunConfiguration configuration = bounded_configuration("voxel_cube_debug_result", true);
    configuration.require_depth_buffer = true;
    configuration.render_bounded_frame = true;
    return run_window(configuration, runner);
}

int run_handcrafted_paddock_scenario(const std::optional<std::filesystem::path>& capture_path) {
    RenderScenarioRunner runner{RenderScenario::handcrafted_paddock, capture_path};
    WindowRunConfiguration configuration = bounded_configuration("paddock_result", true);
    configuration.width = kWindowWidth;
    configuration.height = kWindowHeight;
    configuration.require_depth_buffer = true;
    configuration.render_bounded_frame = true;
    return run_window(configuration, runner);
}

int run_handcrafted_paddock_performance_scenario() {
    RenderScenarioRunner runner{RenderScenario::handcrafted_paddock_performance};
    WindowRunConfiguration configuration =
        bounded_configuration("paddock_performance_result", true);
    configuration.width = 1920;
    configuration.height = 1080;
    configuration.require_depth_buffer = true;
    configuration.performance_warmup_frames = 120;
    configuration.performance_sample_frames = 600;
    configuration.performance_scenario = "handcrafted_paddock_static_v1";
    configuration.performance_budget = core::kLowProfilePerformanceBudget;
    return run_window(configuration, runner);
}

namespace {

int run_handcrafted_paddock_debug_scenario(
    RenderScenario scenario, std::string_view result_name,
    const std::optional<std::filesystem::path>& capture_path) {
    RenderScenarioRunner runner{scenario, capture_path};
    WindowRunConfiguration configuration = bounded_configuration(result_name, true);
    configuration.width = kWindowWidth;
    configuration.height = kWindowHeight;
    configuration.require_depth_buffer = true;
    configuration.render_bounded_frame = true;
    return run_window(configuration, runner);
}

} // namespace

int run_handcrafted_paddock_chunk_bounds_scenario(
    const std::optional<std::filesystem::path>& capture_path) {
    return run_handcrafted_paddock_debug_scenario(RenderScenario::handcrafted_paddock_chunk_bounds,
                                                  "paddock_chunk_bounds_result", capture_path);
}

int run_handcrafted_paddock_face_normals_scenario(
    const std::optional<std::filesystem::path>& capture_path) {
    return run_handcrafted_paddock_debug_scenario(RenderScenario::handcrafted_paddock_face_normals,
                                                  "paddock_face_normals_result", capture_path);
}

int run_handcrafted_paddock_wireframe_scenario(
    const std::optional<std::filesystem::path>& capture_path) {
    return run_handcrafted_paddock_debug_scenario(RenderScenario::handcrafted_paddock_wireframe,
                                                  "paddock_wireframe_result", capture_path);
}

int run_handcrafted_paddock_mesh_statistics_scenario(
    const std::optional<std::filesystem::path>& capture_path) {
    return run_handcrafted_paddock_debug_scenario(
        RenderScenario::handcrafted_paddock_mesh_statistics, "paddock_mesh_statistics_result",
        capture_path);
}

int run_context_high_severity_scenario() {
    NoOpScenarioRunner runner;
    WindowRunConfiguration configuration = bounded_configuration("context_result", true);
    configuration.inject_high_severity_message = true;
    return run_window(configuration, runner);
}

} // namespace wide_eye::platform
