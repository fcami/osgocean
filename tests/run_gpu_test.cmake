# Run one GPU visual regression test.
#
# Usage (called by CTest, not directly):
#   cmake -DOCEAN_EXE=... -DTEST_NAME=... -DGOLDEN_DIR=...
#         -DOCEAN_ARGS=... [-DIMGDIFF=...] [-DUPDATE_GOLDEN=1]
#         -P run_gpu_test.cmake
#
# Runs oceanExample with OSGOCEAN_CAPTURE to produce a PNG,
# then compares against the golden reference using imgdiff.

set(ACTUAL "${GOLDEN_DIR}/${TEST_NAME}_actual.png")
set(GOLDEN "${GOLDEN_DIR}/${TEST_NAME}.png")

# Separate OCEAN_ARGS string into a list
separate_arguments(OCEAN_ARGS_LIST UNIX_COMMAND "${OCEAN_ARGS}")

set(ENV{OSGOCEAN_CAPTURE} "${ACTUAL}")

execute_process(
    COMMAND ${OCEAN_EXE} ${OCEAN_ARGS_LIST}
    WORKING_DIRECTORY ${WORKING_DIR}
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
)

if(NOT EXISTS "${ACTUAL}")
    message(FATAL_ERROR "Capture failed — ${ACTUAL} not created.\n${out}\n${err}")
endif()

if(UPDATE_GOLDEN)
    file(COPY_FILE "${ACTUAL}" "${GOLDEN}")
    message("UPDATED ${GOLDEN}")
    return()
endif()

if(NOT EXISTS "${GOLDEN}")
    message("SKIP ${TEST_NAME}: golden file not found (${GOLDEN}) — run with -DUPDATE_GOLDEN=1")
    return()
endif()

if(NOT IMGDIFF)
    message("SKIP: imgdiff not available — visual comparison skipped")
    return()
endif()

execute_process(
    COMMAND ${IMGDIFF} "${GOLDEN}" "${ACTUAL}" -t ${TOLERANCE}
    RESULT_VARIABLE diff_rc
    OUTPUT_VARIABLE diff_out
)

message("${diff_out}")

if(NOT diff_rc EQUAL 0)
    message(FATAL_ERROR "FAIL ${TEST_NAME}: image differs beyond tolerance")
endif()
