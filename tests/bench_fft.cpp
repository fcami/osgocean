/*
 * FFT performance benchmark.
 *
 * Measures the per-frame cost of setTime + computeHeights +
 * computeDisplacements over many iterations.
 *
 * Not a test — no pass/fail. Just prints timing.
 * Usage: bench_fft [gridSize] [iterations]
 */

#include <osgOcean/FFTSimulation>

#include <osg/Timer>
#include <osg/Vec2f>
#include <osg/Array>

#include <iostream>
#include <cstdlib>

int main(int argc, char** argv)
{
    int gridSize = 64;
    int iterations = 1000;

    if (argc > 1) gridSize = atoi(argv[1]);
    if (argc > 2) iterations = atoi(argv[2]);

    std::cout << "FFT benchmark: grid=" << gridSize
              << " iterations=" << iterations << std::endl;

    srand(42);

    osg::Timer_t t0 = osg::Timer::instance()->tick();

    osgOcean::FFTSimulation sim(
        gridSize,
        osg::Vec2f(1.0f, 1.0f),
        12.0f, 1000.0f, 0.35f, 1e-9f, 256.0f, 10.0f
    );

    osg::Timer_t t1 = osg::Timer::instance()->tick();
    double constructMs = osg::Timer::instance()->delta_m(t0, t1);

    std::cout << "Construction: " << constructMs << " ms" << std::endl;

    osg::ref_ptr<osg::FloatArray> heights = new osg::FloatArray;
    osg::ref_ptr<osg::Vec2Array> displacements = new osg::Vec2Array;

    // Warm up
    sim.setTime(0.0f);
    sim.computeHeights(heights.get());
    sim.computeDisplacements(-2.0f, displacements.get());

    osg::Timer_t tStart = osg::Timer::instance()->tick();

    for (int i = 0; i < iterations; ++i) {
        float t = (float)i * 0.016f;
        sim.setTime(t);
        sim.computeHeights(heights.get());
        sim.computeDisplacements(-2.0f, displacements.get());
    }

    osg::Timer_t tEnd = osg::Timer::instance()->tick();
    double totalMs = osg::Timer::instance()->delta_m(tStart, tEnd);
    double perFrameMs = totalMs / iterations;

    std::cout << "Total: " << totalMs << " ms" << std::endl;
    std::cout << "Per frame: " << perFrameMs << " ms" << std::endl;
    std::cout << "Throughput: " << (1000.0 / perFrameMs) << " frames/sec" << std::endl;

    return 0;
}
