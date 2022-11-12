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

static inline void do_oscillation(AxisEnum a, uint8_t peak, bool apply_delay=false) {
  peak += 24;
  LOOP_L_N(i, peak) {
    //stepper.do_babystep(a,true);
    stepper.do_babystep_without_enable(a,true);
    DELAY_NS(DELAY_48KHZ);
  }
  if(apply_delay){
    DELAY_NS(400000000);
  }
  LOOP_L_N(i, peak) {
    //stepper.do_babystep(a,false);
    stepper.do_babystep_without_enable(a,false);
    DELAY_NS(DELAY_48KHZ);
  }
}

static void do_signature(AxisEnum a) {
  const uint8_t signature[SIGNATURE_LEN] = ARRAY_N(SIGNATURE_LEN, 231, 3, 6, 7, 4, 5, 10, 1, 8, 2, 13,	12, 9 );
  LOOP_L_N(i, SIGNATURE_LEN) {
    do_oscillation(a,signature[i]);
  }
}

/**
 * Send Step/Dir configuration sequence to a single Smart Shaper Driver.
 *
 */
static void configure_smart_shaper(AxisEnum a, const ShapeParams& shaper_params, bool disable_motor) {
  const bool axis_was_enabled = stepper.axis_is_enabled(a);
  if(axis_was_enabled && disable_motor)
    stepper.disable_axis(a);
  do_signature(a);
  do_oscillation(a, 0);
  do_oscillation(a, 2);
  do_oscillation(a, 22);
  do_oscillation(a, 0);
  do_oscillation(a, 64);
  do_oscillation(a, 10);
  do_oscillation(a, 1, true);
  do_oscillation(a, 0);
  if (axis_was_enabled && disable_motor)
    stepper.enable_axis(a);
}

void configure_smartshapers() {
  //if(stepper.shaping_y.gh_shaping_selected())
  ShapeParams p;
  configure_smart_shaper(X_AXIS, p, false);
}

void update_smartshaper(AxisEnum a) {
  //if(stepper.shaping_y.gh_shaping_selected())
  ShapeParams p;
  configure_smart_shaper(a, p, true);
}

#endif
