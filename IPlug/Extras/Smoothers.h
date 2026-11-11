/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/
#pragma once

#include "denormal.h"
#include "IPlugConstants.h"
#include <algorithm> // riite: std::fill (ProcessBlock)
#include <cstring>   // riite: std::memcmp

BEGIN_IPLUG_NAMESPACE

template<typename T, int NC = 1>
class LogParamSmooth
{
private:
  double mA, mB;
  T mOutM1[NC];

public:
  LogParamSmooth(double timeMs = 5., T initialValue = 0.)
  {
    for (auto i = 0; i < NC; i++)
    {
      mOutM1[i] = initialValue;
    }
    
    SetSmoothTime(timeMs, DEFAULT_SAMPLE_RATE);
  }

  // only works for NC = 1
  inline T Process(T input)
  {
    mOutM1[0] = (input * mB) + (mOutM1[0] * mA);
#ifndef OS_IOS
    denormal_fix(&mOutM1[0]);
#endif
    return mOutM1[0];
  }

  inline void SetValue(T value)
  {
    for (auto i = 0; i < NC; i++)
    {
      mOutM1[i] = value;
    }
  }

  inline void SetValues(T values[NC])
  {
    for (auto i = 0; i < NC; i++)
    {
      mOutM1[i] = values[i];
    }
  }

  void SetSmoothTime(double timeMs, double sampleRate)
  {
    static constexpr double TWO_PI = 6.283185307179586476925286766559;
    
    mA = exp(-TWO_PI / (timeMs * 0.001 * sampleRate));
    mB = 1.0 - mA;
  }

  void ProcessBlock(T inputs[NC], T** outputs, int nFrames, int channelOffset = 0)
  {
    const T b = mB;
    const T a = mA;

    // riite: CHANNEL BY CHANNEL, AND A SETTLED CHANNEL IS A FILL (colorapp
    // PERFORMANCE_AUDIT_NOVEMBER_2026 PF6). The channels never touch each other, so
    // taking them one at a time computes the same numbers the sample-by-sample loop did.
    // And a step that gives back the very bits it started from is a fixed point: with the
    // same input every later step gives them back too, so the rest of the block is a copy.
    // An idle synth's smoothers sit there; this was per-sample work on every block.
    for (auto c = 0; c < NC; c++)
    {
      const T input = inputs[channelOffset + c];
      T* pOut = outputs[channelOffset + c];
      T state = mOutM1[c];
      for (auto s = 0; s < nFrames; ++s)
      {
        T output = (input * b) + (state * a);
#ifndef OS_IOS
        denormal_fix(&output);
#endif
        if (std::memcmp(&output, &state, sizeof(T)) == 0)
        {
          std::fill(pOut + s, pOut + nFrames, output);
          break;
        }
        state = output;
        pOut[s] = output;
      }
      mOutM1[c] = state;
    }
  }

} WDL_FIXALIGN;

template<typename T>
class SmoothedGain
{
public:
  SmoothedGain(double smoothingTime = 5.0)
  : mSmoothingTime(smoothingTime)
  {
  }
  
  void ProcessBlock(T** inputs, T** outputs, int nChans, int nFrames, double gainValue)
  {
    for (auto s = 0; s < nFrames; ++s)
    {
      const T smoothedGain = static_cast<T>(mSmoother.Process(gainValue));
      
      for (auto c = 0; c < nChans; c++)
      {
        outputs[c][s] = inputs[c][s] * smoothedGain;
      }
    }
  }
  
  void SetSampleRate(double sampleRate)
  {
    mSmoother.SetSmoothTime(mSmoothingTime, sampleRate);
  }
  
private:
  const double mSmoothingTime;
  LogParamSmooth<double, 1> mSmoother;
} WDL_FIXALIGN;

END_IPLUG_NAMESPACE
