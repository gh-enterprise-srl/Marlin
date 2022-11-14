/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2022 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
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

#include "../../../inc/MarlinConfig.h"

#if ENABLED(ANY_INPUT_SHAPING)

#include "../../gcode.h"
#include "../../../module/stepper.h"
#include "../../../module/stepper/gh.h"

void GcodeSuite::M593_report(const bool forReplay/*=true*/) {
  report_heading_etc(forReplay, F("Input Shaping"));
  #if HAS_SHAPING_X || HAS_GH_SHAPING_X
    SERIAL_ECHOLNPGM("  M593 X"
      " F", stepper.get_shaping_frequency(X_AXIS),
      " D", stepper.get_shaping_damping_ratio(X_AXIS)
    );
  #endif
  #if HAS_SHAPING_Y || HAS_GH_SHAPING_Y
    TERN_(HAS_SHAPING_X, report_echo_start(forReplay));
    SERIAL_ECHOLNPGM("  M593 Y"
      " F", stepper.get_shaping_frequency(Y_AXIS),
      " D", stepper.get_shaping_damping_ratio(Y_AXIS)
    );
  #endif
}

/**
 * M593: Get or Set Input Shaping Parameters
 *  D<factor>    Set the zeta/damping factor. If axes (X, Y, etc.) are not specified, set for all axes.
 *  F<frequency> Set the frequency. If axes (X, Y, etc.) are not specified, set for all axes.
 *  T[map]       Input Shaping type (for GH SmartShaper drivers)-> 20:ZV, 21:MZV, 22:ZVD, EI:23
 *  X<1>         Set the given parameters only for the X axis.
 *  Y<1>         Set the given parameters only for the Y axis.
 */
void GcodeSuite::M593() {
  if (!parser.seen_any()) return M593_report();

  const bool seen_X = TERN0(HAS_ANY_SHAPING_X, parser.seen_test('X')),
             seen_Y = TERN0(HAS_ANY_SHAPING_Y, parser.seen_test('Y')),
             for_X = seen_X || TERN0(HAS_ANY_SHAPING_X, (!seen_X && !seen_Y)),
             for_Y = seen_Y || TERN0(HAS_ANY_SHAPING_Y, (!seen_X && !seen_Y));

  if (parser.seen('D')) {
    const float zeta = parser.value_float();
    if (WITHIN(zeta, 0, 1)) {
      if (for_X) stepper.set_shaping_damping_ratio(X_AXIS, zeta);
      if (for_Y) stepper.set_shaping_damping_ratio(Y_AXIS, zeta);
    }
    else
      SERIAL_ECHO_MSG("?Zeta (D) value out of range (0-1)");
  }

  if (parser.seen('F')) {
    const float freq = parser.value_float();
    #if ENABLED(INPUT_SHAPING)
      constexpr float max_freq = float(uint32_t(STEPPER_TIMER_RATE) / 2) / shaping_time_t(-2);
    #endif
    #if ENABLED(GH_INPUT_SHAPING)
      constexpr float max_freq = 80;
    #endif
    if (freq == 0.0f || ( freq < max_freq && freq > 0)) {
      if (for_X) stepper.set_shaping_frequency(X_AXIS, freq);
      if (for_Y) stepper.set_shaping_frequency(Y_AXIS, freq);
    }
    else
      SERIAL_ECHOLNPGM("?Frequency (F) must be less than ", max_freq, " or 0 to disable");
  }
#if ENABLED(GH_INPUT_SHAPING)
  if (parser.seen('T')) {
    const uint8_t is_type = parser.value_ushort();
    const uint8_t is_type_min = ShapeParams::ISType::ZV_SMART_SHAPER;
    const uint8_t is_type_max = ShapeParams::ISType::EI_SMART_SHAPER;
    if (WITHIN(is_type,is_type_min,is_type_max)) {
      if (for_X) stepper.set_shaping_type(X_AXIS, (ShapeParams::ISType) is_type);
      if (for_Y) stepper.set_shaping_type(Y_AXIS, (ShapeParams::ISType) is_type);
    }
    else
      SERIAL_ECHOLNPGM("?Input Shaping Type (T) out of range (", is_type_min, "-", is_type_max, ")");
  }
#endif

}

#endif
