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
