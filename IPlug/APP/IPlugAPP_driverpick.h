/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

// riite: which ASIO driver a standalone's first run may pick on its own (Windows).
// Header-only so a probe can run the rule against a machine's real driver list.

#include <cctype>
#include <string>
#include <vector>

namespace iplug {

/** True for an ASIO driver that is not one audio interface's own: the onboard Realtek
 *  driver, the wrappers over Windows audio (ASIO4ALL, FlexASIO, ASIO2WASAPI, the
 *  "Generic Low Latency" and "built-in" drivers some DAWs install) and the virtual
 *  routers (Voicemeeter, ReaRoute, JackRouter, Dante Virtual Soundcard). A first run
 *  never picks one of these: each sends the sound somewhere the user did not choose, or
 *  takes the Windows device away from every other program. */
inline bool IsGenericAsioDriver(const std::string& name)
{
  std::string n;
  for (char c : name) n += (char) std::tolower((unsigned char) c);
  static const char* const kWords[] = { "realtek", "generic", "built-in", "asio4all", "flexasio",
                                        "wasapi", "voicemeeter", "rearoute", "jack", "virtual" };
  for (const char* w : kWords)
    if (n.find(w) != std::string::npos) return true;
  return false;
}

/** The driver a first run picks: the one ASIO driver left once the generic ones are set
 *  aside, or "" when none is left or more than one (two interfaces' drivers installed:
 *  which one is plugged in is the user's to say). */
inline std::string PickFirstRunAsioDriver(const std::vector<std::string>& names)
{
  std::string pick;
  int count = 0;
  for (const std::string& s : names)
    if (!s.empty() && !IsGenericAsioDriver(s)) { pick = s; ++count; }
  return count == 1 ? pick : std::string();
}

} // namespace iplug
