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
#include "angular_spring_constraint.h"

// Xefis:
#include <xefis/config/all.h>
#include <xefis/support/math/geometry.h>
#include <xefis/support/simulation/constraints/hinge_precalculation.h>

// Neutrino:
#include <neutrino/numeric.h>

// Standard:
#include <cstddef>


namespace xf::rigid_body {

AngularSpringConstraint::AngularSpringConstraint (HingePrecalculation& hinge, SpringTorqueFunction const spring_torque):
	Constraint (hinge),
	_hinge (hinge),
	_spring_torque (spring_torque)
{
	set_label ("angular spring");
}


ConstraintForces
AngularSpringConstraint::do_constraint_forces (VelocityMoments<WorldSpace> const& vm1, ForceMoments<WorldSpace> const& fm1,
											   VelocityMoments<WorldSpace> const& vm2, ForceMoments<WorldSpace> const& fm2,
											   si::Time const dt) const
{
	auto const angle = _hinge.data().angle;
	auto const pl_1 = Constraint::body_1().placement();
	auto const hinge = pl_1.unbound_transform_to_base (_hinge.body_1_normalized_hinge()) / abs (_hinge.body_1_normalized_hinge());
	// body_1_hinge() and body_2_hinge() should be equal in WorldSpace coordinates.

	auto const force_moments = ForceMoments<WorldSpace> (math::zero, hinge * _spring_torque (angle, hinge, vm1, fm1, vm2, fm2, dt));

	return { +force_moments, -force_moments };
}

} // namespace xf::rigid_body

