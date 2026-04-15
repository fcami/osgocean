include_directories(
    ${osgOcean_SOURCE_DIR}/include
    ${OSG_INCLUDE_DIR}
    ${FFT_INCLUDE_DIR}
)

# --- CPU characterization tests (no GPU required) ---

add_executable(test_fft_characterization test_fft_characterization.cpp)
target_link_libraries(test_fft_characterization
    osgOcean
    optimized ${OSG_LIBRARY}
    debug ${OSG_LIBRARY_DEBUG}
    ${FFT_LIBRARY}
)
add_test(
    NAME fft_characterization
    COMMAND test_fft_characterization
        --golden-dir ${osgOcean_SOURCE_DIR}/tests/golden
    WORKING_DIRECTORY ${osgOcean_SOURCE_DIR}
)
set_tests_properties(fft_characterization PROPERTIES LABELS "cpu")

# --- GPU rendering tests (uses oceanExample with OSGOCEAN_CAPTURE) ---
#
# Each test runs the example app with specific arguments, captures the
# final composited framebuffer to a PNG, and compares against a golden
# reference using imgdiff. The example app's setFinalDrawCallback
# ensures the capture includes all OceanScene post-processing passes.

set(GPU_TEST_SCRIPT ${osgOcean_SOURCE_DIR}/tests/run_gpu_test.cmake)
set(GPU_GOLDEN_DIR ${osgOcean_SOURCE_DIR}/tests/golden)

macro(add_gpu_test TEST_NAME OCEAN_ARGS TOLERANCE)
    add_test(
        NAME ${TEST_NAME}
        COMMAND ${CMAKE_COMMAND}
            -DOCEAN_EXE=$<TARGET_FILE:oceanExample>
            -DTEST_NAME=${TEST_NAME}
            -DGOLDEN_DIR=${GPU_GOLDEN_DIR}
            -DWORKING_DIR=${osgOcean_SOURCE_DIR}
            -DOCEAN_ARGS=${OCEAN_ARGS}
            -DIMGDIFF=${IMGDIFF_EXE}
            -DTOLERANCE=${TOLERANCE}
            -P ${GPU_TEST_SCRIPT}
    )
    set_tests_properties(${TEST_NAME} PROPERTIES
        LABELS "gpu"
        SKIP_RETURN_CODE 77
    )
endmacro()

# Test 1: default CLEAR preset, effects enabled, default camera.
# Validates: full OceanScene pipeline — sky dome reflection,
# Fresnel, wave shading, fog, composited output.
add_gpu_test(ocean_full_effects
    "--windSpeed 12 --waveScale 1e-8"
    0.05)

# Test 2: shaders disabled — fixed pipeline fallback.
# Validates: geometry renders without shaders, material colors.
add_gpu_test(ocean_no_shaders
    "--windSpeed 12 --waveScale 1e-8 --disableShaders"
    0.05)

# Test 3: underwater camera.
# DISABLED: underwater rendering produces inconsistent results on
# Intel Arc and llvmpipe. Do not generate golden files until
# reproduced on a discrete gaming GPU (NVIDIA/AMD).
# add_gpu_test(ocean_underwater
#     "--windSpeed 12 --waveScale 1e-8 --initialCameraPosition 0 0 -10"
#     0.05)
