/* vim:ts=4
 *
 * Copyleft 2012…2023  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef XEFIS__SUPPORT__AERODYNAMICS__PRESSURE_H__INCLUDED
#define XEFIS__SUPPORT__AERODYNAMICS__PRESSURE_H__INCLUDED

// Xefis:
#include <xefis/config/all.h>

// Neutrino:
#include <neutrino/numeric.h>

// Standard:
#include <cstddef>


namespace xf {

constexpr si::Pressure
dynamic_pressure (si::Density const fluid_density, si::Velocity const fluid_speed)
{
	return 0.5 * fluid_density * square (fluid_speed);
}

} // namespace xf

#endif

