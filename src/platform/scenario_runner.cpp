#include "platform/scenario_runner.hpp"

#include "game/camera_controller.hpp"
#include "game/gameplay_replay.hpp"
#include "game/gameplay_simulation.hpp"
#include "platform/visual_tracer_configuration.hpp"
#include "platform/window_runtime.hpp"
#include "render/influence_debug_view.hpp"
#include "render/opengl_renderer.hpp"
#include "render/png_writer.hpp"
#include "voxel/handcrafted_paddock.hpp"

#include <array>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <ostream>
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
    visual_tracer_paddock,
    visual_tracer_debug_paddock,
    visual_tracer_performance_paddock,
    influence_debug_paddock,
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
           scenario == RenderScenario::visual_tracer_paddock ||
           scenario == RenderScenario::visual_tracer_debug_paddock ||
           scenario == RenderScenario::visual_tracer_performance_paddock ||
           scenario == RenderScenario::influence_debug_paddock ||
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
           scenario == RenderScenario::sheep_motion_performance_paddock ||
           scenario == RenderScenario::visual_tracer_paddock ||
           scenario == RenderScenario::visual_tracer_debug_paddock ||
           scenario == RenderScenario::visual_tracer_performance_paddock ||
           scenario == RenderScenario::influence_debug_paddock;
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
    case RenderScenario::visual_tracer_paddock:
    case RenderScenario::visual_tracer_performance_paddock:
    // The overlay explains the scene, so the scene underneath it stays the
    // ordinary lit paddock rather than a second debug mode arguing with it.
    case RenderScenario::influence_debug_paddock:
    case RenderScenario::handcrafted_paddock_performance:
    case RenderScenario::handcrafted_paddock:
    case RenderScenario::triangle:
    case RenderScenario::voxel_cube:
    case RenderScenario::voxel_cube_debug:
        return render::HandcraftedPaddockView::normal;
    case RenderScenario::sheep_motion_debug_paddock:
    case RenderScenario::visual_tracer_debug_paddock:
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

// Fixed review camera for the influence debug view. It is deliberately *not*
// the gameplay follow camera and deliberately not derived from the tick: two
// captures of different ticks have to be comparable, which they are not when the
// eye moves with the dog. That much is unchanged.
//
// What changed is the framing, and why is worth recording. The previous pose,
// `{38, 24, 42} -> {16, 1.5, 21}`, held the whole 32x32 paddock floor in a 16:9
// frame — but the floor is seen as a rotated diamond, so fitting it fits its
// diagonal: 79.5% of the captured frame was clear color, and the overlay, which
// is the entire subject of the capture, landed inside a box under 1% of the
// image and could only be read after cropping and magnifying it. That pose was
// also byte-identical to `kVisualFeasibilityFiveSheep.holdout_camera`, which the
// visual tracer keeps deliberately distant and explicitly does not tune; this
// view had inherited it by copying rather than by choosing it. See
// docs/qa/closed/QA-012-*.md.
//
// The pose below is the same elevated three-quarter direction — 36.6 degrees
// above the horizon, so a ground-plane arrow and the barb rotated about world up
// still read as an arrow, and the lane masts still stand up rather than
// collapsing into each other as they would from overhead — moved in to 14.1
// units. It was chosen by projecting every segment of the retained tick 30, 60,
// and 120 frame dumps through this projection and requiring all of them inside
// the frame with margin, together with the gate, the wall to either side of it,
// and every sheep fixture position any named scenario starts from except the
// one at x=28, z=26. It is not the tracer's holdout and must not be re-synced to
// it: the holdout's distance is that view's whole point.
constexpr render::CameraPose kInfluenceReviewCamera{.eye = {25.0F, 11.0F, 28.0F},
                                                    .target = {16.8F, 2.6F, 20.2F}};

// The frame's own record, in the same key=value idiom the smokes already use.
// It exists so that a capture is attributable: every drawn segment is listed
// with the sheep and the neighbour it belongs to, and the per-sheep block above
// it repeats the published numbers the drawing came from.
void write_influence_debug_dump(std::ostream& out, std::string_view scenario_name,
                                const game::GameplayScenarioDefinition& scenario,
                                const game::GameplaySnapshot& snapshot,
                                const render::InfluenceDebugFrame& frame) {
    out << std::setprecision(17);
    out << "influence_debug_schema=wide-eye.influence-debug-frame\n"
        << "influence_debug_scenario=" << scenario_name << '\n'
        << "influence_debug_scenario_version=" << scenario.version << '\n'
        << "influence_debug_scenario_seed=" << scenario.seed << '\n'
        << "influence_debug_tick=" << frame.tick << '\n'
        << "influence_debug_interpolation=none_published_tick\n"
        << "influence_debug_arrow_scale_seconds_squared="
        << render::kInfluenceArrowScaleSecondsSquared << '\n'
        << "influence_debug_arrow_maximum_length=" << render::kInfluenceArrowMaximumLength << '\n'
        << "influence_debug_segment_count=" << frame.segment_count << '\n'
        << "influence_debug_segment_capacity=" << render::kMaximumInfluenceDebugSegments << '\n'
        << "influence_debug_arrows=" << frame.arrow_count << '\n'
        << "influence_debug_clamped_arrows=" << frame.clamped_arrow_count << '\n'
        << "influence_debug_attraction_links=" << frame.attraction_link_count << '\n'
        << "influence_debug_alignment_links=" << frame.alignment_link_count << '\n'
        << "influence_debug_heading_targets=" << frame.heading_target_count << '\n'
        << "influence_debug_unresolved_neighbor_ids=" << frame.unresolved_neighbor_count << '\n'
        << "influence_debug_flock_markers=" << (frame.flock_markers_present ? "yes" : "no") << '\n';
    for (std::size_t lane = 0; lane < render::kInfluenceChannelCount; ++lane) {
        const auto channel = static_cast<render::InfluenceChannel>(lane);
        const std::array<float, 3> color = render::influence_channel_color(channel);
        out << "influence_lane index=" << lane
            << " channel=" << render::influence_channel_name(channel) << " color=" << color[0]
            << ',' << color[1] << ',' << color[2] << '\n';
    }
    out << "dog x=" << snapshot.dog.position.x << " y=" << snapshot.dog.position.y
        << " z=" << snapshot.dog.position.z << " heading=" << snapshot.dog.heading_radians << '\n';
    if (frame.flock_markers_present) {
        out << "flock centroid_x=" << frame.centroid.x << " centroid_z=" << frame.centroid.z
            << " dog_distance=" << frame.flock_dog.centroid_distance
            << " dog_bearing=" << frame.flock_dog.centroid_bearing_radians
            << " nearest_id=" << frame.flock_dog.nearest_sheep_id
            << " rear_id=" << frame.flock_dog.rear_sheep_id
            << " rear_offset=" << frame.flock_dog.rear_offset
            << " balance_defined=" << (frame.balance_point_defined ? "yes" : "no")
            << " balance_x=" << frame.balance_point.x << " balance_z=" << frame.balance_point.z
            << " dog_behind_flock=" << (frame.dog_behind_flock ? "yes" : "no") << '\n';
    }
    for (std::size_t index = 0; index < snapshot.sheep_count; ++index) {
        const game::SheepState& sheep = snapshot.sheep[index];
        const game::SheepMotionLimitEvidence& motion = snapshot.sheep_motion_limit_evidence[index];
        const game::SheepCombinedInfluenceEvidence& combined =
            snapshot.sheep_combined_influence_evidence[index];
        out << "sheep id=" << sheep.id << " x=" << sheep.position.x << " z=" << sheep.position.z
            << " heading=" << sheep.heading_radians
            << " target_heading=" << motion.motion_heading_radians
            << " target_followed=" << (motion.motion_heading_followed ? "yes" : "no")
            << " heading_change=" << motion.heading_change_radians << " arousal=" << sheep.arousal
            << " behavior=" << game::sheep_behavior_name(sheep.behavior)
            << " temperament=" << game::sheep_temperament_name(sheep.temperament)
            << " summed_magnitude=" << combined.summed_acceleration_magnitude
            << " applied_scale=" << combined.applied_scale << '\n';
    }
    for (std::size_t index = 0; index < frame.segment_count; ++index) {
        const render::DebugSegment& segment = frame.segments[index];
        out << "segment index=" << index
            << " role=" << render::debug_segment_role_name(segment.role)
            << " channel=" << render::influence_channel_name(segment.channel)
            << " subject=" << segment.subject_id << " object=" << segment.object_id
            << " start=" << segment.start[0] << ',' << segment.start[1] << ',' << segment.start[2]
            << " end=" << segment.end[0] << ',' << segment.end[1] << ',' << segment.end[2]
            << " color=" << segment.color[0] << ',' << segment.color[1] << ',' << segment.color[2]
            << '\n';
    }
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
        std::optional<std::filesystem::path> state_dump_path = std::nullopt,
        std::optional<VisualTracerConfiguration> visual_tracer = std::nullopt,
        VisualTracerCamera visual_tracer_camera = VisualTracerCamera::representative)
        : scenario_{scenario}, capture_path_{std::move(capture_path)},
          gameplay_scenario_{gameplay_scenario}, capture_tick_{capture_tick},
          state_dump_path_{std::move(state_dump_path)}, visual_tracer_{visual_tracer},
          visual_tracer_camera_{visual_tracer_camera} {}

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
                simulation_ = std::make_unique<game::GameplaySimulation>(*gameplay_scenario_);
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
                if (scenario_ == RenderScenario::visual_tracer_paddock ||
                    scenario_ == RenderScenario::visual_tracer_debug_paddock ||
                    scenario_ == RenderScenario::visual_tracer_performance_paddock) {
                    if (!visual_tracer_.has_value()) {
                        return WindowFailure{"visual_tracer_configuration", false};
                    }
                    for (std::uint64_t tick = 0; tick < capture_tick_; ++tick) {
                        simulation_->fixed_update(visual_tracer_input_for_tick(tick));
                    }
                    std::cout << "visual_tracer_scene=" << visual_tracer_->id << '\n'
                              << "visual_tracer_scene_version=" << visual_tracer_->version << '\n'
                              << "visual_tracer_route=" << visual_tracer_->route_id << '\n'
                              << "visual_tracer_route_version=" << visual_tracer_->route_version
                              << '\n'
                              << "visual_tracer_tick=" << capture_tick_ << '\n'
                              << "visual_tracer_camera="
                              << visual_tracer_camera_name(visual_tracer_camera_) << '\n'
                              << "visual_tracer_profile=" << visual_tracer_->graphics_profile
                              << '\n';
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
                if (scenario_ == RenderScenario::influence_debug_paddock) {
                    for (std::uint64_t tick = 0; tick < capture_tick_; ++tick) {
                        simulation_->fixed_update(visual_tracer_input_for_tick(tick));
                    }
                    if (state_dump_path_.has_value()) {
                        std::ofstream output{*state_dump_path_, std::ios::binary | std::ios::trunc};
                        write_influence_debug_dump(
                            output, game::gameplay_scenario_name(gameplay_scenario_->id),
                            *gameplay_scenario_, simulation_->current_snapshot(),
                            *build_influence_frame());
                        if (!output) {
                            return WindowFailure{"influence_debug_dump_write", false};
                        }
                        std::cout << "influence_debug_dump_path=" << state_dump_path_->string()
                                  << '\n'
                                  << "influence_debug_dump_schema="
                                     "wide-eye.influence-debug-frame\n";
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
        if (scenario_ != RenderScenario::sheep_motion_performance_paddock &&
            scenario_ != RenderScenario::visual_tracer_performance_paddock) {
            return std::nullopt;
        }
        prepared_gameplay_frame_ = make_gameplay_frame(interpolation_alpha);
        return std::nullopt;
    }

    WindowResult fixed_update(const NamedActionSnapshot& input,
                              double fixed_delta_seconds) override {
        if (simulation_ == nullptr || !camera_.has_value()) {
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
        if (scenario_ == RenderScenario::influence_debug_paddock) {
            return render_influence_debug(window_state);
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
    // The influence view presents the un-interpolated published tick. An overlay
    // of per-tick evidence has to be anchored at the state that produced it: an
    // arrow taken from `current` drawn over a body halfway to `current` would be
    // a half-tick lie, and "the same tick produces the same frame" would depend
    // on the render cadence.
    // The frame is over half a megabyte at the published capacity, so it is
    // filled in caller-owned heap storage rather than returned by value.
    [[nodiscard]] std::unique_ptr<render::InfluenceDebugFrame> build_influence_frame() const {
        auto frame = std::make_unique<render::InfluenceDebugFrame>();
        render::build_influence_debug_frame(simulation_->current_snapshot(),
                                            *gameplay_scenario_, *frame);
        return frame;
    }

    [[nodiscard]] render::HandcraftedPaddockFrame make_influence_debug_scene_frame() const {
        const game::GameplaySnapshot& snapshot = simulation_->current_snapshot();
        return {
            .view = render::HandcraftedPaddockView::normal,
            .camera = kInfluenceReviewCamera,
            .dog =
                render::DogRenderPose{
                    .ground_position = {static_cast<float>(snapshot.dog.position.x),
                                        static_cast<float>(snapshot.dog.position.y),
                                        static_cast<float>(snapshot.dog.position.z)},
                    .heading_radians = static_cast<float>(snapshot.dog.heading_radians),
                },
            .sheep = render::make_sheep_proxy_poses(snapshot),
            .sheep_count = snapshot.sheep_count,
        };
    }

    WindowResult render_influence_debug(const WindowState& window_state) {
        const render::HandcraftedPaddockFrame scene = make_influence_debug_scene_frame();
        renderer_->render_handcrafted_paddock(window_state.pixel_width(),
                                              window_state.pixel_height(), scene);
        const auto influence = build_influence_frame();
        renderer_->render_influence_debug_overlay(
            window_state.pixel_width(), window_state.pixel_height(), scene.camera, *influence);
        std::cout << "scenario=influence_debug\n"
                  << "influence_debug_tick=" << influence->tick << '\n'
                  << "influence_debug_segments=" << influence->segment_count << '\n'
                  << "influence_debug_arrows=" << influence->arrow_count << '\n'
                  << "influence_debug_clamped_arrows=" << influence->clamped_arrow_count << '\n'
                  << "influence_debug_camera_eye=" << scene.camera.eye[0] << ','
                  << scene.camera.eye[1] << ',' << scene.camera.eye[2] << '\n'
                  << "influence_debug_camera_target=" << scene.camera.target[0] << ','
                  << scene.camera.target[1] << ',' << scene.camera.target[2] << '\n'
                  << "influence_debug_draw=issued\n";

        const render::VoxelCubeSample sample = renderer_->sample_handcrafted_paddock_center(
            window_state.pixel_width(), window_state.pixel_height());
        const bool depth_state_matches =
            sample.depth_test_enabled && sample.depth_function_less && sample.depth_write_enabled;
        std::cout << "depth_test_enabled=" << (sample.depth_test_enabled ? "yes" : "no") << '\n'
                  << "depth_function_less=" << (sample.depth_function_less ? "yes" : "no") << '\n'
                  << "depth_write_enabled=" << (sample.depth_write_enabled ? "yes" : "no") << '\n';
        if (!depth_state_matches) {
            return WindowFailure{"influence_debug_depth_state", false};
        }

        const std::optional<render::Rgba8Frame> readback = renderer_->capture_rgba8(
            window_state.pixel_width(), window_state.pixel_height(), std::cerr);
        if (!readback.has_value()) {
            return WindowFailure{"capture_readback", false};
        }
        const std::array<std::size_t, render::kInfluenceChannelCount> lane_pixels =
            render::count_influence_debug_channel_pixels(*readback);
        std::size_t overlay_pixels = 0;
        for (std::size_t lane = 0; lane < lane_pixels.size(); ++lane) {
            overlay_pixels += lane_pixels[lane];
            std::cout << "influence_debug_lane_pixels_"
                      << render::influence_channel_name(static_cast<render::InfluenceChannel>(lane))
                      << '=' << lane_pixels[lane] << '\n';
        }
        const bool frame_matches = render::is_expected_influence_debug_frame(*readback);
        std::cout << "influence_debug_overlay_pixels=" << overlay_pixels << '\n'
                  << "influence_debug_frame_matches=" << (frame_matches ? "yes" : "no") << '\n';
        if (!frame_matches) {
            return WindowFailure{"influence_debug_frame_oracle", false};
        }
        if (capture_path_.has_value()) {
            if (!render::write_png_rgba8(*capture_path_, readback->width, readback->height,
                                         readback->pixels, std::cerr)) {
                return WindowFailure{"capture_write", false};
            }
            std::cout << "capture_path=" << capture_path_->string() << '\n'
                      << "capture_width=" << readback->width << '\n'
                      << "capture_height=" << readback->height << '\n'
                      << "capture_format=png_rgba8\n"
                      << "capture_result=pass\n";
        }
        return std::nullopt;
    }

    [[nodiscard]] render::HandcraftedPaddockFrame
    make_gameplay_frame(double interpolation_alpha) const {
        if (scenario_ == RenderScenario::sheep_motion_paddock ||
            scenario_ == RenderScenario::sheep_motion_debug_paddock ||
            scenario_ == RenderScenario::sheep_motion_performance_paddock) {
            interpolation_alpha = 0.5;
        }
        if (scenario_ == RenderScenario::visual_tracer_paddock ||
            scenario_ == RenderScenario::visual_tracer_debug_paddock ||
            scenario_ == RenderScenario::visual_tracer_performance_paddock) {
            interpolation_alpha = 1.0;
        }
        const game::GameplaySnapshot render_snapshot =
            simulation_->interpolated_snapshot(interpolation_alpha);
        const game::DogState& render_dog = render_snapshot.dog;
        const render::SheepProxyPoseBuffer render_sheep =
            render::make_sheep_proxy_poses(render_snapshot);
        const game::CameraState render_camera = game::interpolate_camera_state(
            previous_camera_state_, camera_->state(), interpolation_alpha);
        const game::CameraPose game_camera = game::camera_pose(render_dog, render_camera);
        render::CameraPose camera_pose{
            .eye = {static_cast<float>(game_camera.eye.x), static_cast<float>(game_camera.eye.y),
                    static_cast<float>(game_camera.eye.z)},
            .target = {static_cast<float>(game_camera.target.x),
                       static_cast<float>(game_camera.target.y),
                       static_cast<float>(game_camera.target.z)},
        };
        if (visual_tracer_.has_value()) {
            camera_pose =
                visual_tracer_camera_pose(*visual_tracer_, visual_tracer_camera_, render_snapshot);
        }
        return {
            .view = paddock_view(scenario_),
            .camera = camera_pose,
            .dog =
                render::DogRenderPose{
                    .ground_position = {static_cast<float>(render_dog.position.x),
                                        static_cast<float>(render_dog.position.y),
                                        static_cast<float>(render_dog.position.z)},
                    .heading_radians = static_cast<float>(render_dog.heading_radians),
                },
            .sheep = render_sheep,
            .sheep_count = render_snapshot.sheep_count,
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
    std::optional<VisualTracerConfiguration> visual_tracer_;
    VisualTracerCamera visual_tracer_camera_ = VisualTracerCamera::representative;
    std::optional<render::OpenGlRenderer> renderer_;
    // Heap, not inline: `GameplaySimulation` is 362 KiB at the authoritative
    // sheep capacity, and every `run_*` entry point holds this runner in a stack
    // frame. Storing it by value put that frame over the default thread stack.
    std::unique_ptr<game::GameplaySimulation> simulation_;
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

int run_visual_tracer_configuration_scenario(std::string_view scene) {
    const auto visual_tracer = find_visual_tracer_configuration(scene);
    if (!visual_tracer.has_value()) {
        std::cerr << "visual_tracer_configuration_result=fail\n"
                  << "failure_stage=visual_tracer_scene_select\n";
        return EXIT_FAILURE;
    }
    const auto gameplay_scenario = game::find_gameplay_scenario(visual_tracer->gameplay_scenario);
    if (!gameplay_scenario.has_value()) {
        std::cerr << "visual_tracer_configuration_result=fail\n"
                  << "failure_stage=visual_tracer_gameplay_scenario_select\n";
        return EXIT_FAILURE;
    }

    std::cout << "visual_tracer_scene=" << visual_tracer->id << '\n'
              << "visual_tracer_scene_version=" << visual_tracer->version << '\n'
              << "visual_tracer_gameplay_scenario=" << visual_tracer->gameplay_scenario << '\n'
              << "visual_tracer_gameplay_scenario_version=" << gameplay_scenario->version << '\n'
              << "visual_tracer_seed=" << gameplay_scenario->seed << '\n'
              << "visual_tracer_route=" << visual_tracer->route_id << '\n'
              << "visual_tracer_route_version=" << visual_tracer->route_version << '\n'
              << "visual_tracer_reference_tick=" << visual_tracer->reference_tick << '\n'
              << "visual_tracer_motion_ticks=" << visual_tracer->motion_ticks[0] << ','
              << visual_tracer->motion_ticks[1] << ',' << visual_tracer->motion_ticks[2] << '\n'
              << "visual_tracer_representative_camera=representative\n"
              << "visual_tracer_holdout_camera=holdout\n"
              << "visual_tracer_holdout_camera_eye=" << visual_tracer->holdout_camera.eye[0] << ','
              << visual_tracer->holdout_camera.eye[1] << ',' << visual_tracer->holdout_camera.eye[2]
              << '\n'
              << "visual_tracer_holdout_camera_target=" << visual_tracer->holdout_camera.target[0]
              << ',' << visual_tracer->holdout_camera.target[1] << ','
              << visual_tracer->holdout_camera.target[2] << '\n'
              << "visual_tracer_profile=" << visual_tracer->graphics_profile << '\n'
              << "visual_tracer_provisional_viewport=" << visual_tracer->provisional_viewport_width
              << 'x' << visual_tracer->provisional_viewport_height << '@'
              << visual_tracer->provisional_refresh_hz << '\n'
              << "visual_tracer_performance_budget_id=" << visual_tracer->performance_budget.id
              << '\n'
              << "visual_tracer_performance_p95_budget_ns="
              << visual_tracer->performance_budget.synchronized_frame_p95_ns << '\n'
              << "visual_tracer_performance_p99_budget_ns="
              << visual_tracer->performance_budget.synchronized_frame_p99_ns << '\n'
              << "visual_tracer_peak_rss_budget_bytes="
              << visual_tracer->performance_budget.peak_rss_bytes << '\n'
              << "visual_tracer_configuration_result=pass\n";
    return EXIT_SUCCESS;
}

int run_visual_tracer_render_scenario(std::string_view scene, std::string_view camera,
                                      std::string_view graphics_profile, int viewport_width,
                                      int viewport_height, int refresh_hz,
                                      std::uint64_t capture_tick, bool debug_view,
                                      const std::filesystem::path& capture_path,
                                      const std::filesystem::path& state_dump_path) {
    const auto visual_tracer = find_visual_tracer_configuration(scene);
    const auto camera_role = find_visual_tracer_camera(camera);
    if (!visual_tracer.has_value() || !camera_role.has_value() ||
        !is_valid_visual_tracer_run(*visual_tracer, graphics_profile, viewport_width,
                                    viewport_height, refresh_hz)) {
        std::cerr << "visual_tracer_render_result=fail\n"
                  << "failure_stage=visual_tracer_run_configuration\n";
        return EXIT_FAILURE;
    }
    const auto definition = game::find_gameplay_scenario(visual_tracer->gameplay_scenario);
    if (!definition.has_value()) {
        std::cerr << "visual_tracer_render_result=fail\n"
                  << "failure_stage=visual_tracer_gameplay_scenario_select\n";
        return EXIT_FAILURE;
    }

    RenderScenarioRunner runner{debug_view ? RenderScenario::visual_tracer_debug_paddock
                                           : RenderScenario::visual_tracer_paddock,
                                capture_path,
                                definition,
                                capture_tick,
                                state_dump_path,
                                visual_tracer,
                                *camera_role};
    WindowRunConfiguration configuration =
        bounded_configuration("visual_tracer_render_result", true);
    configuration.width = viewport_width;
    configuration.height = viewport_height;
    configuration.require_depth_buffer = true;
    configuration.render_bounded_frame = true;
    std::cout << "visual_tracer_viewport_width=" << viewport_width << '\n'
              << "visual_tracer_viewport_height=" << viewport_height << '\n'
              << "visual_tracer_refresh_hz=" << refresh_hz << '\n';
    return run_window(configuration, runner);
}

int run_visual_tracer_performance_scenario(std::string_view scene,
                                           std::string_view graphics_profile, int viewport_width,
                                           int viewport_height, int refresh_hz) {
    const auto visual_tracer = find_visual_tracer_configuration(scene);
    if (!visual_tracer.has_value() ||
        !is_valid_visual_tracer_run(*visual_tracer, graphics_profile, viewport_width,
                                    viewport_height, refresh_hz)) {
        std::cerr << "visual_tracer_performance_result=fail\n"
                  << "failure_stage=visual_tracer_run_configuration\n";
        return EXIT_FAILURE;
    }
    const auto definition = game::find_gameplay_scenario(visual_tracer->gameplay_scenario);
    if (!definition.has_value()) {
        std::cerr << "visual_tracer_performance_result=fail\n"
                  << "failure_stage=visual_tracer_gameplay_scenario_select\n";
        return EXIT_FAILURE;
    }

    RenderScenarioRunner runner{RenderScenario::visual_tracer_performance_paddock,
                                std::nullopt,
                                definition,
                                visual_tracer->reference_tick,
                                std::nullopt,
                                visual_tracer,
                                VisualTracerCamera::representative};
    WindowRunConfiguration configuration =
        bounded_configuration("visual_tracer_performance_result", true);
    configuration.width = viewport_width;
    configuration.height = viewport_height;
    configuration.require_depth_buffer = true;
    configuration.performance_warmup_frames = 120;
    configuration.performance_sample_frames = 600;
    configuration.performance_scenario = visual_tracer->id;
    configuration.performance_budget = visual_tracer->performance_budget;
    std::cout << "visual_tracer_viewport_width=" << viewport_width << '\n'
              << "visual_tracer_viewport_height=" << viewport_height << '\n'
              << "visual_tracer_refresh_hz=" << refresh_hz << '\n';
    return run_window(configuration, runner);
}

int run_influence_debug_render_scenario(
    std::string_view gameplay_scenario, std::uint64_t capture_tick,
    const std::optional<std::filesystem::path>& capture_path,
    const std::optional<std::filesystem::path>& frame_dump_path) {
    const auto definition = game::find_gameplay_scenario(gameplay_scenario);
    if (!definition.has_value()) {
        std::cerr << "influence_debug_result=fail\n"
                  << "failure_stage=influence_debug_scenario_select\n";
        return EXIT_FAILURE;
    }
    RenderScenarioRunner runner{RenderScenario::influence_debug_paddock, capture_path, definition,
                                capture_tick, frame_dump_path};
    WindowRunConfiguration configuration = bounded_configuration("influence_debug_result", true);
    configuration.width = 1920;
    configuration.height = 1080;
    configuration.require_depth_buffer = true;
    configuration.render_bounded_frame = true;
    return run_window(configuration, runner);
}

int run_influence_debug_dump_scenario(std::string_view gameplay_scenario,
                                      std::uint64_t capture_tick,
                                      const std::optional<std::filesystem::path>& frame_dump_path) {
    const auto definition = game::find_gameplay_scenario(gameplay_scenario);
    if (!definition.has_value()) {
        std::cerr << "influence_debug_dump_result=fail\n"
                  << "failure_stage=influence_debug_scenario_select\n";
        return EXIT_FAILURE;
    }
    // Heap, not stack: `GameplaySimulation` is around 115 KiB, which is the
    // defect QA-002 recorded.
    const auto simulation = std::make_unique<game::GameplaySimulation>(*definition);
    for (std::uint64_t tick = 0; tick < capture_tick; ++tick) {
        simulation->fixed_update(visual_tracer_input_for_tick(tick));
    }
    const auto frame = std::make_unique<render::InfluenceDebugFrame>();
    render::build_influence_debug_frame(simulation->current_snapshot(), *definition, *frame);
    if (frame_dump_path.has_value()) {
        std::ofstream output{*frame_dump_path, std::ios::binary | std::ios::trunc};
        write_influence_debug_dump(output, game::gameplay_scenario_name(definition->id),
                                   *definition, simulation->current_snapshot(), *frame);
        if (!output) {
            std::cerr << "influence_debug_dump_result=fail\n"
                      << "failure_stage=influence_debug_dump_write\n";
            return EXIT_FAILURE;
        }
        std::cout << "influence_debug_dump_path=" << frame_dump_path->string() << '\n';
    } else {
        write_influence_debug_dump(std::cout, game::gameplay_scenario_name(definition->id),
                                   *definition, simulation->current_snapshot(), *frame);
    }
    const bool within_capacity = frame->segment_count <= frame->segments.size();
    const bool neighbors_resolve = frame->unresolved_neighbor_count == 0;
    std::cout << "influence_debug_dump_schema=wide-eye.influence-debug-frame\n"
              << "influence_debug_dump_scenario=" << game::gameplay_scenario_name(definition->id)
              << '\n'
              << "influence_debug_dump_tick=" << frame->tick << '\n'
              << "influence_debug_dump_segments=" << frame->segment_count << '\n'
              << "influence_debug_dump_within_capacity=" << (within_capacity ? "yes" : "no") << '\n'
              << "influence_debug_dump_unresolved_neighbor_ids=" << frame->unresolved_neighbor_count
              << '\n'
              << "influence_debug_dump_result="
              << (within_capacity && neighbors_resolve ? "pass" : "fail") << '\n';
    return within_capacity && neighbors_resolve ? EXIT_SUCCESS : EXIT_FAILURE;
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
