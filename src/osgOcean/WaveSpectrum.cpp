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
