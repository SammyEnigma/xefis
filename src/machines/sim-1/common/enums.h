/* vim:ts=4
 *
 * Copyleft 2008…2023  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef XEFIS__MACHINES__SIM_1__COMMON__ENUMS_H__INCLUDED
#define XEFIS__MACHINES__SIM_1__COMMON__ENUMS_H__INCLUDED

// Neutrino:
#include <neutrino/si/si.h>
#include <neutrino/math/math.h>


namespace sim1 {

enum PilotControlsMode: uint8_t
{
	// Joystick directly affects control surface deflections:
	DirectControl,
	// Joystick controls angular acceleration (with virtual angular drag):
	AngularAccelerationControl,
	// Joystick controls angular velocity:
	AngularVelocityControl,
	// Load control:
	LoadControl,
};

} // namespace sim1

#endif

