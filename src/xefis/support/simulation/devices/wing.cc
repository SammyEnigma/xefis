/* vim:ts=4
 *
 * Copyleft 2019  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

// Local:
#include "wing.h"

// Xefis:
#include <xefis/config/all.h>
#include <xefis/support/math/geometry.h>
#include <xefis/support/math/triangle.h>
#include <xefis/support/math/triangulation.h>
#include <xefis/support/simulation/rigid_body/various_shapes.h>

// Standard:
#include <cstddef>


namespace xf::sim {

Wing::Wing (Airfoil const& airfoil, si::Density const material_density):
	Body (calculate_body_com_mass_moments (airfoil, material_density)),
	_airfoil (airfoil)
{
	set_shape (rigid_body::make_airfoil_shape ({
		.spline = _airfoil.spline(),
		.chord_length = _airfoil.chord_length(),
		.wing_length = _airfoil.wing_length(),
		.with_bottom = true,
		.with_top = true,
	}));
}


void
Wing::update_external_forces (Atmosphere const* atmosphere)
{
	if (atmosphere)
	{
		// Rotations: TODO Perhaps don't do these calculations if it's just multiplying by 1?
		auto const world_to_ecef = RotationQuaternion<ECEFSpace, WorldSpace> (math::identity);
		auto const ecef_to_world = RotationQuaternion<WorldSpace, ECEFSpace> (math::identity);
		auto const body_to_airfoil_spline = RotationQuaternion<AirfoilSplineSpace, BodyCOM> (math::identity);
		auto const airfoil_spline_to_body = RotationQuaternion<BodyCOM, AirfoilSplineSpace> (math::identity);
		auto const world_to_body = placement().base_to_body_rotation();
		// ECEF → WorldSpace → BodyCOM → AirfoilSplineSpace:
		RotationQuaternion<AirfoilSplineSpace, ECEFSpace> ecef_to_spline_transform = body_to_airfoil_spline * world_to_body * ecef_to_world;

		auto const body_position_in_ecef = world_to_ecef * placement().position();
		auto const body_velocity_in_ecef = world_to_ecef * velocity_moments<WorldSpace>().velocity();

		auto ecef_air = atmosphere->air_at (body_position_in_ecef);
		ecef_air.velocity -= body_velocity_in_ecef;
		auto const airfoil_spline_air = ecef_to_spline_transform * ecef_air;
		auto const body_air = airfoil_spline_to_body * airfoil_spline_air;

		// Center of pressure Wrench:
		auto const spline_aeroforces_at_origin = _airfoil.aerodynamic_forces (airfoil_spline_air);
		auto const body_aeroforces_at_origin = airfoil_spline_to_body * spline_aeroforces_at_origin.forces;

		// Compute 'at COM' values:
		auto const lift_force = body_aeroforces_at_origin.lift;
		auto const drag_force = body_aeroforces_at_origin.drag;
		auto const pitching_moment = body_aeroforces_at_origin.pitching_moment;
		auto const center_of_pressure = body_aeroforces_at_origin.center_of_pressure + origin<BodyCOM>();

		// New parameters converted to BodyCOM:
		_airfoil_aerodynamic_parameters = {
			.air = body_air,
			.reynolds_number = spline_aeroforces_at_origin.reynolds_number,
			.true_air_speed = spline_aeroforces_at_origin.true_air_speed,
			.angle_of_attack = spline_aeroforces_at_origin.angle_of_attack,
			.forces = {
				.lift = lift_force,
				.drag = drag_force,
				.pitching_moment = pitching_moment,
				.center_of_pressure = center_of_pressure,
			},
		};

		apply_impulse (ForceMoments<BodyCOM> (lift_force, pitching_moment), center_of_pressure);
		apply_impulse (ForceMoments<BodyCOM> (drag_force, math::zero), center_of_pressure);
	}
}


MassMomentsAtArm<BodyCOM>
Wing::calculate_body_com_mass_moments (Airfoil const& airfoil, si::Density const material_density)
{
	// Well, let AirfoilSplineSpace and BodyCOM be actually the same, so an identity rotation:
	auto const rotation = RotationQuaternion<BodyCOM, AirfoilSplineSpace> (math::identity);
	return rotation * calculate_mass_moments_at_arm<AirfoilSplineSpace> (airfoil, material_density);
}

} // namespace xf::sim

