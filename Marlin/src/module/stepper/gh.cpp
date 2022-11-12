/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

/**
 * stepper/gh.cpp
 * GH Enterprise Smart Shaper configuration tools
 */

#include "../../inc/MarlinConfig.h"

#if ENABLED(GH_INPUT_SHAPING)
#include "gh.h"
#include "../../HAL/shared/Delay.h"

#define SIGNATURE_LEN 13
#define DELAY_48KHZ 20800

enum GHInputShapingAlgorithms
{
  Disabled,
  ZV,
  MZV,
  ZVD,
  EI
};

enum VibrationFreqMultipliers
{
  M1,
  M0_1
};

enum GHTMC2225SpreadPin
{
  SpreadPinLow, // StealthChopMode Left untouched
  SpreadPinHigh // StealthChopMode Inverted
};

static inline void do_oscillation(AxisEnum a, uint8_t peak, bool apply_delay = false)
{
  peak += 24;
  LOOP_L_N(i, peak)
  {
    // stepper.do_babystep(a,true);
    stepper.do_babystep_for_gh_smartshaper(a, true);
    DELAY_NS(DELAY_48KHZ);
  }
  if (apply_delay)
  {
    DELAY_NS(400000000);
  }
  LOOP_L_N(i, peak)
  {
    // stepper.do_babystep(a,false);
    stepper.do_babystep_for_gh_smartshaper(a, false);
    DELAY_NS(DELAY_48KHZ);
  }
}

static void do_signature(AxisEnum a)
{
  const uint8_t signature[SIGNATURE_LEN] = ARRAY_N(SIGNATURE_LEN, 231, 3, 6, 7, 4, 5, 10, 1, 8, 2, 13, 12, 9);
  LOOP_L_N(i, SIGNATURE_LEN)
  {
    do_oscillation(a, signature[i]);
  }
}

static inline GHInputShapingAlgorithms istype_to_gh_type(ShapeParams::ISType t)
{
  switch (t)
  {
  case ShapeParams::ZV_SMART_SHAPER:
    return GHInputShapingAlgorithms::ZV;
  case ShapeParams::MZV_SMART_SHAPER:
    return GHInputShapingAlgorithms::MZV;
  case ShapeParams::ZVD_SMART_SHAPER:
    return GHInputShapingAlgorithms::ZVD;
  case ShapeParams::EI_SMART_SHAPER:
    return GHInputShapingAlgorithms::EI;
  default:
    return GHInputShapingAlgorithms::Disabled;
  }
}

/**
 * Send Step/Dir configuration sequence to a single Smart Shaper Driver.
 *
 */
static void configure_smart_shaper(AxisEnum a, ShapeParams::ISType is_type, float freq, uint8_t zeta_perc, bool disable_motor)
{
  const bool axis_was_enabled = stepper.axis_is_enabled(a);

  // disable motors if requested
  if (axis_was_enabled && disable_motor)
    stepper.disable_axis(a);

  // Prepare command parameters
  const uint8_t command_id = 0;
  const bool input_shaping_disabled = (freq == 0);
  const uint8_t algo = (input_shaping_disabled ? GHInputShapingAlgorithms::Disabled : istype_to_gh_type(is_type));
  const uint8_t freq_par = (input_shaping_disabled ? 0 : (uint8_t) freq);
  const uint8_t sampling_freq_KHz = 64;
  GHTMC2225SpreadPin spread_pin = SpreadPinLow;
  const uint8_t sync_oscillation = 0;

  // Signature
  do_signature(a);

  // Command
  do_oscillation(a, command_id);
  do_oscillation(a, algo);
  do_oscillation(a, freq_par);
  do_oscillation(a, VibrationFreqMultipliers::M1); // Freq = Freq * 1; No scaling factor applied
  do_oscillation(a, sampling_freq_KHz);
  do_oscillation(a, zeta_perc);
  do_oscillation(a, spread_pin, true);
  do_oscillation(a, sync_oscillation);

  // restore previous enable status
  if (axis_was_enabled && disable_motor)
    stepper.enable_axis(a);
}

void program_smartshaper(AxisEnum a, bool disable_motor_while_configuring)
{

  // resonance frequency
  const float freq = stepper.get_shaping_frequency(a);
  // damping ratio
  uint8_t zeta_perc = stepper.get_shaping_damping_ratio(a) * 100;
  zeta_perc = (zeta_perc > 99 ? 99 : zeta_perc); // force <= 99%
  // input shaping type
  ShapeParams::ISType is_type = stepper.get_shaping_type(a);

  configure_smart_shaper(a, is_type, freq, zeta_perc, disable_motor_while_configuring);
}

#endif
