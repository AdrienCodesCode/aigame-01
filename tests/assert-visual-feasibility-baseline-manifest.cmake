cmake_minimum_required(VERSION 3.28)

if(NOT DEFINED MANIFEST OR NOT EXISTS "${MANIFEST}")
    message(FATAL_ERROR "Visual-feasibility baseline manifest does not exist: ${MANIFEST}")
endif()

file(READ "${MANIFEST}" manifest_json)

function(require_json expected_value)
    set(path ${ARGN})
    string(JSON actual_value ERROR_VARIABLE json_error GET "${manifest_json}" ${path})
    if(json_error OR NOT "${actual_value}" STREQUAL "${expected_value}")
        message(FATAL_ERROR "Manifest field ${path} expected '${expected_value}', got '${actual_value}': ${json_error}")
    endif()
endfunction()

require_json("wide-eye.artifact-manifest" schema)
require_json("1" schema_version)
require_json("visual-feasibility-baseline-v1" packet_version)
require_json("pass" result)
require_json("inventory.json" platform)
require_json("configuration.json" configuration)
require_json("measurements.json" measurements)
require_json("source-hashes.json" source_hashes)
require_json("review.md" review_document)
require_json("visual-rubric.md" rubric)

get_filename_component(packet_directory "${MANIFEST}" DIRECTORY)
foreach(file IN ITEMS inventory.json configuration.json measurements.json source-hashes.json
                      visual-rubric.md review.md)
    if(NOT EXISTS "${packet_directory}/${file}")
        message(FATAL_ERROR "Baseline packet is missing ${file}")
    endif()
endforeach()

file(READ "${packet_directory}/configuration.json" configuration_json)
string(JSON scene GET "${configuration_json}" scene)
string(JSON profile GET "${configuration_json}" profile)
string(JSON reference_tick GET "${configuration_json}" reference_tick)
if(NOT scene STREQUAL "visual-feasibility-five-sheep-v1" OR
   NOT profile STREQUAL "visual-feasibility-reference-high-v1" OR
   NOT reference_tick EQUAL 30)
    message(FATAL_ERROR "Baseline scene/profile/reference tick drifted")
endif()

file(READ "${packet_directory}/measurements.json" measurements_json)
string(JSON gl_renderer GET "${measurements_json}" gl_renderer)
string(JSON actual_gl GET "${measurements_json}" actual_gl)
string(JSON high_severity GET "${measurements_json}" gl_debug_high_severity_messages)
string(JSON within_budget GET "${measurements_json}" within_performance_budget)
string(JSON capture_repeat GET "${measurements_json}" same_state_capture_repeat)
string(JSON state_repeat GET "${measurements_json}" same_state_dump_repeat)
if(NOT gl_renderer MATCHES "NVIDIA GeForce RTX 4070 Ti" OR
   NOT actual_gl STREQUAL "4.6" OR NOT high_severity STREQUAL "0" OR
   NOT within_budget STREQUAL "yes" OR NOT capture_repeat STREQUAL "yes" OR
   NOT state_repeat STREQUAL "yes")
    message(FATAL_ERROR "Baseline hardware, GL, repeatability, diagnostics, or budget evidence failed")
endif()

set(required_stages configure-release build-release ctest-release visual-tracer-configuration
                    representative-normal representative-normal-repeat representative-debug
                    holdout-normal holdout-debug motion-tick-1 motion-tick-30 motion-tick-90
                    visual-tracer-performance)
string(JSON command_count LENGTH "${manifest_json}" commands)
set(found_stages)
math(EXPR command_last "${command_count} - 1")
foreach(index RANGE 0 ${command_last})
    string(JSON stage GET "${manifest_json}" commands ${index} stage)
    list(APPEND found_stages "${stage}")
endforeach()
foreach(stage IN LISTS required_stages)
    if(NOT stage IN_LIST found_stages)
        message(FATAL_ERROR "Baseline manifest is missing command stage ${stage}")
    endif()
endforeach()

set(required_roles log inventory configuration measurements source_hashes visual_rubric review
                   representative_normal representative_normal_repeat representative_debug
                   holdout_normal holdout_debug state_reference release_binaries
                   motion_tick_1 motion_tick_30 motion_tick_90
                   motion_state_tick_1 motion_state_tick_30 motion_state_tick_90)
string(JSON artifact_count LENGTH "${manifest_json}" artifacts)
set(found_roles)
math(EXPR artifact_last "${artifact_count} - 1")
foreach(index RANGE 0 ${artifact_last})
    string(JSON role GET "${manifest_json}" artifacts ${index} role)
    string(JSON path GET "${manifest_json}" artifacts ${index} path)
    string(JSON expected_sha GET "${manifest_json}" artifacts ${index} sha256)
    set(full_path "${packet_directory}/${path}")
    if(NOT EXISTS "${full_path}")
        message(FATAL_ERROR "Manifest artifact is missing: ${full_path}")
    endif()
    file(SHA256 "${full_path}" actual_sha)
    string(TOLOWER "${actual_sha}" actual_sha)
    if(NOT actual_sha STREQUAL expected_sha)
        message(FATAL_ERROR "Manifest artifact hash mismatch for ${path}")
    endif()
    list(APPEND found_roles "${role}")
endforeach()
foreach(role IN LISTS required_roles)
    if(NOT role IN_LIST found_roles)
        message(FATAL_ERROR "Baseline manifest is missing artifact role ${role}")
    endif()
endforeach()

message("visual_feasibility_manifest_result=pass")
