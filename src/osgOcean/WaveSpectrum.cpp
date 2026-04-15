/*
* This source file is part of the osgOcean library
*
* Copyright (C) 2009 Kim Bale
* Copyright (C) 2009 The University of Hull, UK
*
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU Lesser General Public License as published by the Free Software
* Foundation; either version 3 of the License, or (at your option) any later
* version.

* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
* http://www.gnu.org/copyleft/lesser.txt.
*/

#include <osgOcean/WaveSpectrum>
#include <cmath>

using namespace osgOcean;

const float PhillipsSpectrum::GRAVITY  = 9.81f;
const float PhillipsSpectrum::GRAVITY2 = 9.81f * 9.81f;

PhillipsSpectrum::PhillipsSpectrum(
    const osg::Vec2f& windDir,
    float windSpeed,
    float waveScale,
    float tileRes,
    float reflDamping,
    int fourierSize)
    : _windDir(windDir)
    , _windSpeed4(windSpeed * windSpeed * windSpeed * windSpeed)
    , _A(float(fourierSize) * waveScale)
    , _maxWave(_windSpeed4 / GRAVITY2)
    , _reflDampFactor(reflDamping)
{
}

float PhillipsSpectrum::operator()(const osg::Vec2f& K) const
{
    float k2 = K.length2();

    if (k2 == 0.f)
        return 0.f;

    float k4 = k2 * k2;

    float KdotW = K * _windDir;

    float KdotWhat = KdotW * KdotW / k2;

    float eterm = exp(-GRAVITY2 / (k2 * _windSpeed4)) / k4;

    const float damping = 0.000001f;

    float specResult = _A * eterm * KdotWhat * exp(-k2 * _maxWave * damping);

    if (KdotW < 0.f)
        specResult *= _reflDampFactor;

    return specResult;
}

// --- JONSWAP ---

const float JONSWAPSpectrum::GRAVITY = 9.81f;

JONSWAPSpectrum::JONSWAPSpectrum(
    const osg::Vec2f& windDir,
    float windSpeed,
    float fetch,
    float waveScale,
    float tileRes,
    float reflDamping,
    int fourierSize,
    float gamma)
    : _windDir(windDir)
    , _windSpeed(windSpeed)
    , _fetch(fetch)
    , _A(float(fourierSize) * waveScale)
    , _reflDampFactor(reflDamping)
    , _gamma(gamma)
{
    // Peak angular frequency from JONSWAP parameterization
    // wp = 22 * (g^2 / (U * F))^(1/3)  where U=windSpeed, F=fetch
    // Simplified: wp = 2*PI * 3.5 * (g/U) * (g*F/U^2)^(-0.33)
    _wp = 22.0f * pow(GRAVITY * GRAVITY / (_windSpeed * _fetch), 1.0f/3.0f);
}

float JONSWAPSpectrum::operator()(const osg::Vec2f& K) const
{
    float k2 = K.length2();

    if (k2 == 0.f)
        return 0.f;

    float k = sqrt(k2);

    // Deep water dispersion: w = sqrt(g * k)
    float w = sqrt(GRAVITY * k);

    if (w < 1e-6f)
        return 0.f;

    // Pierson-Moskowitz base spectrum
    // S_PM(w) = (alpha * g^2 / w^5) * exp(-5/4 * (wp/w)^4)
    float alpha = 0.0081f; // Phillips constant (empirical)
    float w5 = w * w * w * w * w;
    float wpOverW = _wp / w;
    float wpOverW4 = wpOverW * wpOverW * wpOverW * wpOverW;
    float pm = (alpha * GRAVITY * GRAVITY / w5) * exp(-1.25f * wpOverW4);

    // JONSWAP peak enhancement
    // gamma^r where r = exp(-(w - wp)^2 / (2 * sigma^2 * wp^2))
    float sigma = (w <= _wp) ? 0.07f : 0.09f;
    float dwNorm = (w - _wp) / (sigma * _wp);
    float r = exp(-0.5f * dwNorm * dwNorm);
    float peakEnhancement = pow(_gamma, r);

    float jonswap = pm * peakEnhancement;

    // Convert from frequency spectrum S(w) to wavenumber spectrum S(K)
    // S(K) = S(w) * dw/dk / (2*PI*k)
    // For deep water: dw/dk = g / (2*w)
    // S(K) = S(w) * g / (2*w) / (2*PI*k)
    float dwdk = GRAVITY / (2.0f * w);
    float sK = jonswap * dwdk / (2.0f * osg::PI * k);

    // Apply directional spreading (cos^2)
    float KdotW = K * _windDir;
    float KdotWhat = KdotW * KdotW / k2;

    float specResult = _A * sK * KdotWhat;

    if (KdotW < 0.f)
        specResult *= _reflDampFactor;

    return specResult;
}
