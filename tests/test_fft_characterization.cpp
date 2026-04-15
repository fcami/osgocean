/*
 * Characterization tests for FFTSimulation.
 *
 * These tests pin the current FFT output for fixed inputs so that
 * refactoring (Sequence 2) and enhancements (Sequence 3) can be
 * verified against a known baseline.
 *
 * No GPU, no window, no display required. Pure CPU computation.
 */

#include <osgOcean/FFTSimulation>

#include <osg/Vec2f>
#include <osg/Array>

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static const char* GOLDEN_DIR = nullptr;
static bool UPDATE_GOLDEN = false;

// -----------------------------------------------------------------
// Golden file I/O
// -----------------------------------------------------------------

static std::string goldenPath(const std::string& name)
{
    std::string dir = GOLDEN_DIR ? GOLDEN_DIR : "tests/golden";
    return dir + "/" + name;
}

static bool writeFloats(const std::string& path, const float* data, unsigned int count)
{
    std::ofstream f(path.c_str());
    if (!f) return false;
    f.precision(9);
    for (unsigned int i = 0; i < count; ++i)
        f << data[i] << "\n";
    return f.good();
}

static bool readFloats(const std::string& path, std::vector<float>& out)
{
    std::ifstream f(path.c_str());
    if (!f) return false;
    float v;
    while (f >> v)
        out.push_back(v);
    return !out.empty();
}

static bool writeVec2s(const std::string& path, const osg::Vec2* data, unsigned int count)
{
    std::ofstream f(path.c_str());
    if (!f) return false;
    f.precision(9);
    for (unsigned int i = 0; i < count; ++i)
        f << data[i].x() << " " << data[i].y() << "\n";
    return f.good();
}

static bool readVec2s(const std::string& path, std::vector<osg::Vec2>& out)
{
    std::ifstream f(path.c_str());
    if (!f) return false;
    float x, y;
    while (f >> x >> y)
        out.push_back(osg::Vec2(x, y));
    return !out.empty();
}

// -----------------------------------------------------------------
// Comparison
// -----------------------------------------------------------------

static bool compareFloats(const float* actual, const std::vector<float>& expected,
                          unsigned int count, float tolerance, const std::string& label)
{
    if (expected.size() != count) {
        std::cerr << "FAIL " << label << ": expected " << expected.size()
                  << " values, got " << count << std::endl;
        return false;
    }
    float maxErr = 0.0f;
    unsigned int maxIdx = 0;
    for (unsigned int i = 0; i < count; ++i) {
        float err = std::fabs(actual[i] - expected[i]);
        if (err > maxErr) { maxErr = err; maxIdx = i; }
    }
    if (maxErr > tolerance) {
        std::cerr << "FAIL " << label << ": max error " << maxErr
                  << " at index " << maxIdx
                  << " (actual=" << actual[maxIdx]
                  << " expected=" << expected[maxIdx] << ")" << std::endl;
        return false;
    }
    std::cout << "PASS " << label << ": max error " << maxErr << std::endl;
    return true;
}

static bool compareVec2s(const osg::Vec2* actual, const std::vector<osg::Vec2>& expected,
                         unsigned int count, float tolerance, const std::string& label)
{
    if (expected.size() != count) {
        std::cerr << "FAIL " << label << ": expected " << expected.size()
                  << " values, got " << count << std::endl;
        return false;
    }
    float maxErr = 0.0f;
    unsigned int maxIdx = 0;
    for (unsigned int i = 0; i < count; ++i) {
        float ex = std::fabs(actual[i].x() - expected[i].x());
        float ey = std::fabs(actual[i].y() - expected[i].y());
        float err = std::max(ex, ey);
        if (err > maxErr) { maxErr = err; maxIdx = i; }
    }
    if (maxErr > tolerance) {
        std::cerr << "FAIL " << label << ": max error " << maxErr
                  << " at index " << maxIdx << std::endl;
        return false;
    }
    std::cout << "PASS " << label << ": max error " << maxErr << std::endl;
    return true;
}

// -----------------------------------------------------------------
// Tests
// -----------------------------------------------------------------

static bool test_heights()
{
    // Fixed seed + fixed parameters = reproducible output.
    srand(42);
    osgOcean::FFTSimulation sim(
        64,                         // fourierSize
        osg::Vec2f(1.0f, 1.0f),    // windDir
        12.0f,                      // windSpeed
        1000.0f,                    // depth
        0.35f,                      // reflectionDamping
        1e-9f,                      // waveScale
        256.0f,                     // tileRes
        10.0f                       // loopTime
    );

    sim.setTime(1.0f);

    osg::ref_ptr<osg::FloatArray> heights = new osg::FloatArray;
    sim.computeHeights(heights.get());

    unsigned int n = heights->size();
    if (n != 64 * 64) {
        std::cerr << "FAIL heights: expected 4096 values, got " << n << std::endl;
        return false;
    }

    std::string path = goldenPath("fft_heights_64_t1.txt");

    if (UPDATE_GOLDEN) {
        writeFloats(path, &(*heights)[0], n);
        std::cout << "UPDATED " << path << std::endl;
        return true;
    }

    std::vector<float> expected;
    if (!readFloats(path, expected)) {
        std::cerr << "SKIP heights: golden file not found (" << path << ")"
                  << " — run with --update-golden to create" << std::endl;
        return true; // skip, not fail
    }

    return compareFloats(&(*heights)[0], expected, n, 1e-6f, "heights");
}

static bool test_displacements()
{
    srand(42);
    osgOcean::FFTSimulation sim(
        64, osg::Vec2f(1.0f, 1.0f), 12.0f, 1000.0f, 0.35f, 1e-9f, 256.0f, 10.0f
    );

    sim.setTime(1.0f);

    osg::ref_ptr<osg::Vec2Array> displacements = new osg::Vec2Array;
    sim.computeDisplacements(-2.0f, displacements.get());

    unsigned int n = displacements->size();
    if (n != 64 * 64) {
        std::cerr << "FAIL displacements: expected 4096 values, got " << n << std::endl;
        return false;
    }

    std::string path = goldenPath("fft_displacements_64_t1.txt");

    if (UPDATE_GOLDEN) {
        writeVec2s(path, &(*displacements)[0], n);
        std::cout << "UPDATED " << path << std::endl;
        return true;
    }

    std::vector<osg::Vec2> expected;
    if (!readVec2s(path, expected)) {
        std::cerr << "SKIP displacements: golden file not found (" << path << ")"
                  << " — run with --update-golden to create" << std::endl;
        return true;
    }

    return compareVec2s(&(*displacements)[0], expected, n, 1e-6f, "displacements");
}

static bool test_determinism()
{
    // Two independent simulations with identical parameters and
    // identical random seed must produce identical output.
    // RandUtils uses rand(), so we control the seed with srand().
    srand(42);
    osgOcean::FFTSimulation sim1(
        64, osg::Vec2f(1.0f, 1.0f), 12.0f, 1000.0f, 0.35f, 1e-9f, 256.0f, 10.0f
    );
    srand(42);
    osgOcean::FFTSimulation sim2(
        64, osg::Vec2f(1.0f, 1.0f), 12.0f, 1000.0f, 0.35f, 1e-9f, 256.0f, 10.0f
    );

    sim1.setTime(5.0f);
    sim2.setTime(5.0f);

    osg::ref_ptr<osg::FloatArray> h1 = new osg::FloatArray;
    osg::ref_ptr<osg::FloatArray> h2 = new osg::FloatArray;
    sim1.computeHeights(h1.get());
    sim2.computeHeights(h2.get());

    if (h1->size() != h2->size()) {
        std::cerr << "FAIL determinism: sizes differ" << std::endl;
        return false;
    }

    for (unsigned int i = 0; i < h1->size(); ++i) {
        if ((*h1)[i] != (*h2)[i]) {
            std::cerr << "FAIL determinism: differ at index " << i
                      << " (" << (*h1)[i] << " vs " << (*h2)[i] << ")" << std::endl;
            return false;
        }
    }

    std::cout << "PASS determinism: " << h1->size() << " values identical" << std::endl;
    return true;
}

static bool test_nonzero()
{
    // Sanity: the ocean with 12 m/s wind should produce nonzero heights.
    srand(42);
    osgOcean::FFTSimulation sim(
        64, osg::Vec2f(1.0f, 1.0f), 12.0f, 1000.0f, 0.35f, 1e-9f, 256.0f, 10.0f
    );

    sim.setTime(1.0f);

    osg::ref_ptr<osg::FloatArray> heights = new osg::FloatArray;
    sim.computeHeights(heights.get());

    float maxH = 0.0f;
    for (unsigned int i = 0; i < heights->size(); ++i) {
        float h = std::fabs((*heights)[i]);
        if (h > maxH) maxH = h;
    }

    if (maxH < 1e-10f) {
        std::cerr << "FAIL nonzero: max height is " << maxH << std::endl;
        return false;
    }

    std::cout << "PASS nonzero: max height " << maxH << std::endl;
    return true;
}

// -----------------------------------------------------------------

int main(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--update-golden")
            UPDATE_GOLDEN = true;
        else if (std::string(argv[i]) == "--golden-dir" && i + 1 < argc)
            GOLDEN_DIR = argv[++i];
    }

    int failures = 0;

    if (!test_nonzero())       ++failures;
    if (!test_determinism())   ++failures;
    if (!test_heights())       ++failures;
    if (!test_displacements()) ++failures;

    if (failures > 0) {
        std::cerr << failures << " test(s) FAILED" << std::endl;
        return 1;
    }

    std::cout << "All FFT characterization tests passed" << std::endl;
    return 0;
}
