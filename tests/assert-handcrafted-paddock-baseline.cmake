cmake_minimum_required(VERSION 3.28)

foreach(required_variable IN ITEMS MANIFEST EXPECTED_MANIFEST_SHA256
                                  EXPECTED_REVIEW_SHA256 EXPECTED_CAPTURE_SHA256)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "Paddock-baseline validation requires ${required_variable}")
    endif()
endforeach()
if(NOT EXISTS "${MANIFEST}")
    message(FATAL_ERROR "Paddock manifest does not exist: ${MANIFEST}")
endif()

file(READ "${MANIFEST}" manifest_json)
file(SHA256 "${MANIFEST}" manifest_sha)
string(TOLOWER "${manifest_sha}" manifest_sha)
if(NOT manifest_sha STREQUAL EXPECTED_MANIFEST_SHA256)
    message(
        FATAL_ERROR
        "Paddock manifest hash mismatch: ${manifest_sha} vs ${EXPECTED_MANIFEST_SHA256}"
    )
endif()

function(require_json expected_value)
    set(path ${ARGN})
    string(JSON actual_value ERROR_VARIABLE json_error GET "${manifest_json}" ${path})
    if(json_error)
        message(FATAL_ERROR "Paddock manifest field ${path} is invalid: ${json_error}")
    endif()
    if(NOT "${actual_value}" STREQUAL "${expected_value}")
        message(
            FATAL_ERROR
            "Paddock manifest field ${path} expected '${expected_value}', got '${actual_value}'"
        )
    endif()
endfunction()

require_json("wide-eye.artifact-manifest" schema)
require_json("1" schema_version)
require_json("tracer1-handcrafted-paddock-baseline-v1" packet_version)
require_json("pass" result)
require_json("accept" owner_verdict)
require_json("native-windows" platform name)
require_json("handcrafted_paddock" scenario name)
require_json("1" scenario version)
require_json("tracer1_fixed_paddock_blockout" scenario camera)
require_json("960" scenario viewport width)
require_json("540" scenario viewport height)
require_json("development-blockout" scenario graphics_profile)
require_json("4" scene_metrics chunks)
require_json("1746" scene_metrics occupied_blocks)
require_json("2754" scene_metrics faces)
require_json("11016" scene_metrics vertices)
require_json("16524" scene_metrics indices)
require_json("ON" verification repeat_capture_byte_identical)
require_json("0" verification gl_debug_high_severity_messages)

string(JSON artifact_count ERROR_VARIABLE artifact_error LENGTH "${manifest_json}" artifacts)
if(artifact_error OR NOT artifact_count EQUAL 1)
    message(FATAL_ERROR "Paddock baseline must retain exactly one accepted capture")
endif()
require_json("normal_capture" artifacts 0 role)
require_json("normal-frame.png" artifacts 0 path)
require_json("image/png" artifacts 0 media_type)
require_json("2074363" artifacts 0 bytes)
require_json("${EXPECTED_CAPTURE_SHA256}" artifacts 0 sha256)

get_filename_component(packet_directory "${MANIFEST}" DIRECTORY)
set(capture_path "${packet_directory}/normal-frame.png")
if(NOT EXISTS "${capture_path}")
    message(FATAL_ERROR "Accepted paddock capture is missing: ${capture_path}")
endif()
file(SHA256 "${capture_path}" capture_sha)
string(TOLOWER "${capture_sha}" capture_sha)
if(NOT capture_sha STREQUAL EXPECTED_CAPTURE_SHA256)
    message(
        FATAL_ERROR
        "Accepted paddock capture hash mismatch: ${capture_sha} vs ${EXPECTED_CAPTURE_SHA256}"
    )
endif()

require_json("review.md" review_document)
set(review_path "${packet_directory}/review.md")
if(NOT EXISTS "${review_path}")
    message(FATAL_ERROR "Accepted paddock review is missing: ${review_path}")
endif()
file(READ "${review_path}" review_markdown)
file(SHA256 "${review_path}" review_sha)
string(TOLOWER "${review_sha}" review_sha)
if(NOT review_sha STREQUAL EXPECTED_REVIEW_SHA256)
    message(
        FATAL_ERROR
        "Accepted paddock review hash mismatch: ${review_sha} vs ${EXPECTED_REVIEW_SHA256}"
    )
endif()
if(NOT review_markdown MATCHES "manifest.json / ${manifest_sha}")
    message(FATAL_ERROR "Accepted paddock review does not record the manifest hash")
endif()

string(FIND "${review_markdown}" "- [x] **Accept**" accept_marker)
string(FIND "${review_markdown}" "- [x] **Revise**" revise_marker)
string(FIND "${review_markdown}" "- [x] **Reject**" reject_marker)
if(accept_marker EQUAL -1)
    message(FATAL_ERROR "Accepted paddock baseline lacks an explicit Accept verdict")
endif()
if(NOT revise_marker EQUAL -1 OR NOT reject_marker EQUAL -1)
    message(FATAL_ERROR "Accepted paddock baseline records more than one verdict")
endif()
if(NOT review_markdown MATCHES
   "\\*\\*Owner observation and required follow-up:\\*\\*[^\r\n]+")
    message(FATAL_ERROR "Accepted paddock baseline lacks the owner's observation")
endif()
if(NOT review_markdown MATCHES "\\*\\*Owner/date:\\*\\*[^\r\n]+")
    message(FATAL_ERROR "Accepted paddock baseline lacks the owner/date")
endif()

message("handcrafted_paddock_baseline_result=pass")
