cmake_minimum_required(VERSION 3.28)

if(NOT DEFINED MANIFEST)
    message(FATAL_ERROR "Artifact-manifest validation requires MANIFEST")
endif()
if(NOT EXISTS "${MANIFEST}")
    message(FATAL_ERROR "Artifact manifest does not exist: ${MANIFEST}")
endif()

file(READ "${MANIFEST}" manifest_json)
file(SHA256 "${MANIFEST}" manifest_sha)
string(TOLOWER "${manifest_sha}" manifest_sha)

if(DEFINED EXPECTED_MANIFEST_SHA256 AND
   NOT manifest_sha STREQUAL EXPECTED_MANIFEST_SHA256)
    message(
        FATAL_ERROR
        "Manifest hash mismatch: ${manifest_sha} vs ${EXPECTED_MANIFEST_SHA256}"
    )
endif()

function(require_json expected_value)
    set(path ${ARGN})
    string(JSON actual_value ERROR_VARIABLE json_error GET "${manifest_json}" ${path})
    if(json_error)
        message(FATAL_ERROR "Manifest field ${path} is missing or invalid: ${json_error}")
    endif()
    if(NOT "${actual_value}" STREQUAL "${expected_value}")
        message(
            FATAL_ERROR
            "Manifest field ${path} expected '${expected_value}', got '${actual_value}'"
        )
    endif()
endfunction()

require_json("wide-eye.artifact-manifest" schema)
require_json("1" schema_version)
require_json("native-windows" platform name)
require_json("voxel_cube_smoke" scenario name)
require_json("1" scenario version)

string(JSON packet_version GET "${manifest_json}" packet_version)
if(NOT packet_version STREQUAL "tracer0-cube-smoke-v1" AND
   NOT packet_version STREQUAL "tracer0-cube-review-v1" AND
   NOT packet_version STREQUAL "tracer0-cube-sanitizer-v1")
    message(FATAL_ERROR "Unsupported artifact packet version: ${packet_version}")
endif()

if(DEFINED EXPECTED_PRESET)
    require_json("${EXPECTED_PRESET}" build configure_preset)
    require_json("${EXPECTED_PRESET}" build build_preset)
    require_json("${EXPECTED_PRESET}" build test_preset)
endif()

if(packet_version STREQUAL "tracer0-cube-sanitizer-v1")
    require_json("dev-sanitized" build configure_preset)
    require_json("dev-sanitized" build build_preset)
    require_json("dev-sanitized" build test_preset)
    require_json("AddressSanitizer" build sanitizers 0)
endif()

string(JSON source_commit ERROR_VARIABLE source_commit_error GET "${manifest_json}" source commit)
string(LENGTH "${source_commit}" source_commit_length)
if(source_commit_error OR NOT source_commit MATCHES "^[0-9a-f]+$" OR
   NOT source_commit_length EQUAL 40)
    message(FATAL_ERROR "Manifest must record a full lowercase Git commit")
endif()
string(JSON worktree_state GET "${manifest_json}" source worktree_state)
if(NOT worktree_state MATCHES "^(clean|dirty)$")
    message(FATAL_ERROR "Manifest must record whether its source worktree was clean or dirty")
endif()

if(DEFINED EXPECTED_RESULT)
    require_json("${EXPECTED_RESULT}" result)
endif()

string(JSON command_count ERROR_VARIABLE command_error LENGTH "${manifest_json}" commands)
if(command_error OR command_count LESS 1)
    message(FATAL_ERROR "Manifest must retain at least one executed command")
endif()
set(found_stages)
math(EXPR command_last "${command_count} - 1")
foreach(index RANGE 0 ${command_last})
    string(JSON command_stage GET "${manifest_json}" commands ${index} stage)
    list(APPEND found_stages "${command_stage}")
endforeach()
foreach(required_stage IN ITEMS configure build ctest triangle-smoke voxel-cube-smoke
                                capture-one capture-two)
    if(NOT required_stage IN_LIST found_stages)
        message(FATAL_ERROR "Manifest is missing required command stage: ${required_stage}")
    endif()
endforeach()

string(JSON packet_result GET "${manifest_json}" result)
if((packet_version STREQUAL "tracer0-cube-review-v1" OR
    packet_version STREQUAL "tracer0-cube-sanitizer-v1") AND
   packet_result STREQUAL "pass")
    if(NOT debug-capture IN_LIST found_stages)
        message(FATAL_ERROR "Passing capture manifest is missing command stage: debug-capture")
    endif()
endif()

string(JSON artifact_count ERROR_VARIABLE artifact_error LENGTH "${manifest_json}" artifacts)
if(artifact_error OR artifact_count LESS 4)
    message(FATAL_ERROR "Manifest must retain log, configuration, state, and source hashes")
endif()

get_filename_component(packet_directory "${MANIFEST}" DIRECTORY)
set(required_roles log configuration state source_hashes)
set(found_roles)
math(EXPR artifact_last "${artifact_count} - 1")
foreach(index RANGE 0 ${artifact_last})
    string(JSON artifact_role GET "${manifest_json}" artifacts ${index} role)
    string(JSON artifact_path GET "${manifest_json}" artifacts ${index} path)
    string(JSON expected_sha GET "${manifest_json}" artifacts ${index} sha256)
    set(full_path "${packet_directory}/${artifact_path}")
    if(NOT EXISTS "${full_path}")
        message(FATAL_ERROR "Manifest artifact is missing: ${full_path}")
    endif()
    file(SHA256 "${full_path}" actual_sha)
    string(TOLOWER "${actual_sha}" actual_sha)
    if(NOT actual_sha STREQUAL expected_sha)
        message(
            FATAL_ERROR
            "Manifest artifact hash mismatch for ${artifact_path}: ${actual_sha} vs ${expected_sha}"
        )
    endif()
    list(APPEND found_roles "${artifact_role}")
endforeach()

foreach(required_role IN LISTS required_roles)
    if(NOT required_role IN_LIST found_roles)
        message(FATAL_ERROR "Manifest is missing required artifact role: ${required_role}")
    endif()
endforeach()

if(DEFINED REQUIRE_CAPTURE AND REQUIRE_CAPTURE)
    if(NOT normal_capture IN_LIST found_roles)
        message(FATAL_ERROR "Manifest is missing its normal capture")
    endif()
endif()

if((packet_version STREQUAL "tracer0-cube-review-v1" AND packet_result STREQUAL "pass") OR
   (DEFINED REQUIRE_DEBUG_CAPTURE AND REQUIRE_DEBUG_CAPTURE))
    if(NOT debug_capture IN_LIST found_roles)
        message(FATAL_ERROR "Manifest is missing its debug capture")
    endif()
endif()

if(DEFINED REQUIRE_REVIEW AND REQUIRE_REVIEW)
    string(JSON review_document ERROR_VARIABLE review_error GET "${manifest_json}" review_document)
    if(review_error OR NOT review_document STREQUAL "review.md")
        message(FATAL_ERROR "Manifest must identify review.md as its review document")
    endif()
    set(review_path "${packet_directory}/${review_document}")
    if(NOT EXISTS "${review_path}")
        message(FATAL_ERROR "Visual-review document is missing: ${review_path}")
    endif()
    file(READ "${review_path}" review_markdown)
    if(NOT review_markdown MATCHES "manifest.json / ${manifest_sha}")
        message(FATAL_ERROR "Visual-review document does not record the manifest SHA-256")
    endif()
    if(DEFINED EXPECTED_REVIEW_SHA256)
        file(SHA256 "${review_path}" review_sha)
        string(TOLOWER "${review_sha}" review_sha)
        if(NOT review_sha STREQUAL EXPECTED_REVIEW_SHA256)
            message(
                FATAL_ERROR
                "Visual-review hash mismatch: ${review_sha} vs ${EXPECTED_REVIEW_SHA256}"
            )
        endif()
    endif()
endif()

if(DEFINED REQUIRE_ACCEPTED_REVIEW AND REQUIRE_ACCEPTED_REVIEW)
    if(NOT DEFINED REQUIRE_REVIEW OR NOT REQUIRE_REVIEW)
        message(FATAL_ERROR "Accepted-review validation also requires REQUIRE_REVIEW=ON")
    endif()

    string(FIND "${review_markdown}" "- [x] **Accept**" accept_marker)
    string(FIND "${review_markdown}" "- [x] **Revise**" revise_marker)
    string(FIND "${review_markdown}" "- [x] **Reject**" reject_marker)
    if(accept_marker EQUAL -1)
        message(FATAL_ERROR "Accepted baseline does not record an explicit Accept verdict")
    endif()
    if(NOT revise_marker EQUAL -1 OR NOT reject_marker EQUAL -1)
        message(FATAL_ERROR "Accepted baseline records more than one verdict")
    endif()
    if(NOT review_markdown MATCHES
       "\\*\\*Owner observation and required follow-up:\\*\\*[^\r\n]+")
        message(FATAL_ERROR "Accepted baseline must record the owner's observation")
    endif()
    if(NOT review_markdown MATCHES "\\*\\*Owner/date:\\*\\*[^\r\n]+")
        message(FATAL_ERROR "Accepted baseline must record its owner/date")
    endif()
endif()

if(DEFINED REQUIRE_REPEAT_CAPTURE AND REQUIRE_REPEAT_CAPTURE)
    if(NOT repeat_capture IN_LIST found_roles)
        message(FATAL_ERROR "Manifest is missing its failed repeat capture")
    endif()
endif()

if(DEFINED EXPECTED_FAILURE_STAGE)
    require_json("${EXPECTED_FAILURE_STAGE}" failure stage)
endif()

message("artifact_manifest_result=pass")
