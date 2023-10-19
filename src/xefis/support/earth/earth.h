/* vim:ts=4
 *
 * Copyleft 2012…2016  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef XEFIS__SUPPORT__EARTH__EARTH_H__INCLUDED
#define XEFIS__SUPPORT__EARTH__EARTH_H__INCLUDED

// Xefis:
#include <xefis/config/all.h>
#include <xefis/support/math/geometry.h>
#include <xefis/support/nature/constants.h>

// Standard:
#include <cstddef>


namespace xf {

/**
 * Compute distance between two sets of coordinates on Earth.
 * Result is in sphere radius units.
 */
[[nodiscard]]
si::Angle::Value
haversine (si::LonLat const& a, si::LonLat const& b);


/**
 * Convenience function.
 * Compute distance between two sets of coordinates on Earth.
 * Result is in nautical miles.
 */
[[nodiscard]]
si::Length
haversine_earth (si::LonLat const& a, si::LonLat const& b);


/**
 * Initial bearing when flying orthodrome (great circle path)
 * to another point. For final bearing, reverse the arguments.
 * Result is in range [-180_deg, +180_deg].
 */
[[nodiscard]]
si::Angle
initial_bearing (si::LonLat const& a, si::LonLat const& b);


/**
 * Compute angle between two great arcs on a sphere.
 * Arcs are given by three points, the second one lies on the intersection.
 * Result is in degrees.
 */
[[nodiscard]]
si::Angle
great_arcs_angle (si::LonLat const& a, si::LonLat const& common, si::LonLat const& b);


[[nodiscard]]
std::string
to_dms (si::Angle, bool three_digits);


[[nodiscard]]
std::string
to_latitude_dms (si::Angle);


[[nodiscard]]
std::string
to_longitude_dms (si::Angle);


/**
 * Mean value for two angles on a circle.
 */
[[nodiscard]]
si::Angle
mean (si::Angle a, si::Angle b);


/*
 * Implementation
 */


[[nodiscard]]
inline si::Length
haversine_earth (si::LonLat const& a, si::LonLat const& b)
{
	return haversine (a, b) * kEarthMeanRadius;
}

} // namespace xf

#endif

