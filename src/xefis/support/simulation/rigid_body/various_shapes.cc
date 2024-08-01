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
#include "various_shapes.h"

// Xefis:
#include <xefis/config/all.h>
#include <xefis/support/math/geometry.h>
#include <xefis/support/simulation/rigid_body/shape_vertex.h>

// Neutrino:
#include <neutrino/numeric.h>

// Standard:
#include <algorithm>
#include <cstddef>


namespace xf::rigid_body {

Shape
make_centered_cube_shape (si::Length const edge_length, ShapeMaterial const& material)
{
	return make_centered_cube_shape (SpaceLength<BodySpace> { edge_length, edge_length, edge_length }, material);
}


Shape
make_centered_cube_shape (SpaceLength<BodySpace> const& dimensions, ShapeMaterial const& material)
{
	Shape shape;
	auto const x = 0.5 * dimensions[0];
	auto const y = 0.5 * dimensions[1];
	auto const z = 0.5 * dimensions[2];

	shape.triangles() = {
		// Front:
		{ { +x, +y, +z }, { -x, +y, +z }, { -x, -y, +z } },
		{ { +x, +y, +z }, { -x, -y, +z }, { +x, -y, +z } },
		// Right:
		{ { +x, +y, +z }, { +x, -y, +z }, { +x, -y, -z } },
		{ { +x, +y, +z }, { +x, -y, -z }, { +x, +y, -z } },
		// Top:
		{ { +x, +y, +z }, { +x, +y, -z }, { -x, +y, -z } },
		{ { +x, +y, +z }, { -x, +y, -z }, { -x, +y, +z } },
		// Back:
		{ { -x, -y, -z }, { +x, +y, -z }, { +x, -y, -z } },
		{ { -x, -y, -z }, { -x, +y, -z }, { +x, +y, -z } },
		// Left:
		{ { -x, -y, -z }, { -x, +y, +z }, { -x, +y, -z } },
		{ { -x, -y, -z }, { -x, -y, +z }, { -x, +y, +z } },
		// Bottom:
		{ { -x, -y, -z }, { +x, -y, +z }, { -x, -y, +z } },
		{ { -x, -y, -z }, { +x, -y, -z }, { +x, -y, +z } },
	};

	auto b = shape.triangles().begin();
	auto e = shape.triangles().end();

	set_planar_normals (b, e);
	set_material (b, e, material);

	return shape;
}


Shape
make_centered_cube_shape (xf::MassMoments<BodySpace> const& mm, ShapeMaterial const& material)
{
	// Assuming center of mass position is 0.

	auto const k = mm.mass() / 12;
	auto const inv_double_k = 1 / (2 * k);
	auto const I = mm.moment_of_inertia(); // Assuming it's ortogonalized
	auto const d0 = I[0, 0];
	auto const d1 = I[1, 1];
	auto const d2 = I[2, 2];

	si::Length const x = sqrt ((-d0 +d1 +d2) * inv_double_k);
	si::Length const y = sqrt ((+d0 -d1 +d2) * inv_double_k);
	si::Length const z = sqrt ((+d0 +d1 -d2) * inv_double_k);

	return make_centered_cube_shape (SpaceLength<BodySpace> { x, y, z }, material);
}


Shape
make_centered_sphere_shape (si::Length const radius, size_t slices, size_t stacks,
							Range<si::Angle> const h_range, Range<si::Angle> const v_range,
							ShapeMaterial const& material, MakeSphereMaterialCallback const setup_material)
{
	slices = std::max<size_t> (slices, 3);
	stacks = std::max<size_t> (stacks, 2);

	si::Angle const dh = h_range.extent() / slices;
	si::Angle const dv = v_range.extent() / stacks;

	Shape shape;
	si::Angle angle_v = v_range.min();

	for (size_t iv = 0; iv < stacks; ++iv, angle_v += dv)
	{
		Shape::TriangleStrip& strip = shape.triangle_strips().emplace_back();
		si::Angle angle_h = h_range.max();

		for (size_t ih = 0; ih < slices + 1; ++ih, angle_h += dh)
		{
			// Not the most effective (could reuse vertices), but who cares.
			auto const get_vector = [](si::Angle v, si::Angle const h)
			{
				v -= 90_deg;

				auto const w = sin (v);
				auto const x = w * sin (h);
				auto const y = w * cos (h);
				auto const z = cos (v);

				return SpaceVector<double, BodySpace> (x, y, z);
			};

			auto const p1 = get_vector (angle_v, angle_h);
			auto const p2 = get_vector (angle_v + dv, angle_h);

			if (setup_material)
			{
				auto p1_material = material;
				setup_material (p1_material, angle_v);
				strip.emplace_back (p1 * radius, p1, p1_material);

				auto p2_material = material;
				setup_material (p2_material, angle_v + dv);
				strip.emplace_back (p2 * radius, p2, p2_material);
			}
			else
			{
				strip.emplace_back (p1 * radius, p1, material);
				strip.emplace_back (p2 * radius, p2, material);
			}
		}
	}

	shape.for_all_vertices ([] (ShapeVertex& v) {
		v.set_normal (v.position() / 1_m);
	});

	return shape;
}


Shape
make_cylinder_shape (CylinderShapeParameters const& params, ShapeMaterial const& material)
{
	auto num_faces = params.num_faces;

	if (num_faces < 3)
		num_faces = 3;

	Shape shape;
	Shape::TriangleStrip& strip = shape.triangle_strips().emplace_back();
	std::optional<Shape::TriangleFan> face1;
	std::optional<Shape::TriangleFan> face2;

	if (params.with_front_and_back)
	{
		face1.emplace();
		face1->emplace_back (SpaceLength<BodySpace> (0_m, 0_m, 0_m), SpaceVector<double, BodySpace> (0.0, 0.0, -1.0), material);
		face2.emplace();
		face2->emplace_back (SpaceLength<BodySpace> (0_m, 0_m, params.length), SpaceVector<double, BodySpace> (0.0, 0.0, +1.0), material);
	}

	si::Angle const da = 360_deg / num_faces;
	si::Angle angle = 0_deg;

	for (size_t i = 0; i < num_faces + 1; ++i, angle += da)
	{
		auto const x = sin (angle);
		auto const y = cos (angle);
		auto const x_len = params.radius * x;
		auto const y_len = params.radius * y;
		SpaceVector<double, BodySpace> const normal (x, y, 0);
		SpaceLength<BodySpace> const p1 (x_len, y_len, 0_m);
		SpaceLength<BodySpace> const p2 (x_len, y_len, params.length);

		strip.emplace_back (p1, normal, material);
		strip.emplace_back (p2, normal, material);

		if (params.with_front_and_back)
		{
			face1->emplace_back (p1, SpaceVector<double, BodySpace> (0.0, 0.0, -1.0), material);
			face2->emplace_back (p2, SpaceVector<double, BodySpace> (0.0, 0.0, +1.0), material);
		}
	}

	if (params.with_front_and_back)
	{
		// Reverse order to keep the face facing outside:
		std::reverse (std::next (face2->begin()), face2->end());
		shape.triangle_fans() = { std::move (*face1), std::move (*face2) };
	}

	return shape;
}


Shape
make_cone_shape (si::Length const length, si::Length const radius, size_t num_faces, bool with_bottom,
				 ShapeMaterial const& material)
{
	if (num_faces < 3)
		num_faces = 3;

	Shape shape;
	Shape::TriangleStrip& cone_strip = shape.triangle_strips().emplace_back();
	std::optional<Shape::TriangleFan> bottom_fan;

	if (with_bottom)
	{
		bottom_fan.emplace();
		bottom_fan->emplace_back (SpaceLength<BodySpace> (0_m, 0_m, 0_m), SpaceVector<double, BodySpace> (0.0, 0.0, -1.0), material);
	}

	si::Angle const da = 360_deg / num_faces;
	si::Angle angle = 0_deg;

	for (size_t i = 0; i < num_faces + 1; ++i, angle += da)
	{
		using std::sin;
		using std::atan;

		auto const y = cos (angle);
		auto const x = sin (angle);
		auto const z = sin (atan (radius / length));
		SpaceVector<double, BodySpace> const normal (x, y, z);
		SpaceLength<BodySpace> const p1 (x * radius, y * radius, 0_m);
		SpaceLength<BodySpace> const p2 (0_m, 0_m, length);

		cone_strip.emplace_back (p1, normal, material);
		cone_strip.emplace_back (p2, normal, material);

		if (with_bottom)
			bottom_fan->emplace_back (p1, SpaceVector<double, BodySpace> (0.0, 0.0, -1.0), material);
	}

	if (with_bottom)
		shape.triangle_fans() = { std::move (*bottom_fan) };

	return shape;
}


Shape
make_solid_circle (si::Length const radius, size_t num_slices, ShapeMaterial const& material)
{
	if (num_slices < 3)
		num_slices = 3;

	Shape shape;
	Shape::TriangleFan& fan = shape.triangle_fans().emplace_back();
	fan.emplace_back (SpaceLength<BodySpace> (0_m, 0_m, 0_m), SpaceVector<double, BodySpace> (0.0, 0.0, 1.0), material);

	si::Angle const da = 360_deg / num_slices;
	si::Angle angle = 0_deg;

	for (size_t i = 0; i < num_slices + 1; ++i, angle += da)
	{
		auto const y = sin (angle);
		auto const x = cos (angle);

		fan.emplace_back (SpaceLength<BodySpace> (x * radius, y * radius, 0_m), SpaceVector<double, BodySpace> (0.0, 0.0, +1.0), material);
	}

	return shape;
}


Shape
make_airfoil_shape (AirfoilSpline const& spline, si::Length const chord_length, si::Length wing_length, bool with_front_and_back,
					ShapeMaterial const& material)
{
	Shape shape;
	Shape::TriangleStrip& strip = shape.triangle_strips().emplace_back();
	std::optional<Shape::TriangleFan> face1;
	std::optional<Shape::TriangleFan> face2;

	if (with_front_and_back)
	{
		face1.emplace();
		face1->emplace_back (SpaceLength<BodySpace> (0_m, 0_m, 0_m), SpaceVector<double, BodySpace> (0.0, 0.0, -1.0), material);
		face2.emplace();
		face2->emplace_back (SpaceLength<BodySpace> (0_m, 0_m, wing_length), SpaceVector<double, BodySpace> (0.0, 0.0, +1.0), material);
	}

	auto const n_points = spline.points().size();

	for (ptrdiff_t i = neutrino::to_signed (n_points) + 1; i > 0; --i)
	{
		auto const prev_point = spline.points()[wrap_array_index (i - 1, n_points)];
		auto const point = spline.points()[to_unsigned (i) % n_points];
		auto const next_point = spline.points()[wrap_array_index (i + 1, n_points)];

		auto const x_len = chord_length * point[0];
		auto const y_len = chord_length * point[1];
		SpaceLength<BodySpace> const p1 (x_len, y_len, 0_m);
		SpaceLength<BodySpace> const p2 (x_len, y_len, wing_length);

		SpaceVector<double, BodySpace> const z_versor (0, 0, 1);
		SpaceVector<double, BodySpace> const k_towards_prev = SpaceVector<double, BodySpace> (prev_point[0], prev_point[1], 0) - SpaceVector<double, BodySpace> (point[0], point[1], 0);
		SpaceVector<double, BodySpace> const k_towards_next = SpaceVector<double, BodySpace> (next_point[0], next_point[1], 0) - SpaceVector<double, BodySpace> (point[0], point[1], 0);
		SpaceVector<double, BodySpace> const normal_with_prev (cross_product (z_versor, k_towards_prev));
		SpaceVector<double, BodySpace> const normal_with_next (cross_product (k_towards_next, z_versor));
		SpaceVector<double, BodySpace> const normal = normalized (normal_with_prev + normal_with_next);

		strip.emplace_back (p1, normal, material);
		strip.emplace_back (p2, normal, material);

		if (with_front_and_back)
		{
			face1->emplace_back (p1, SpaceVector<double, BodySpace> (0.0, 0.0, -1.0), material);
			face2->emplace_back (p2, SpaceVector<double, BodySpace> (0.0, 0.0, +1.0), material);
		}
	}

	if (with_front_and_back)
	{
		// Reverse order to keep the face facing outside:
		std::reverse (std::next (face2->begin()), face2->end());
		shape.triangle_fans() = { std::move (*face1), std::move (*face2) };
	}

	return shape;
}


Shape
make_center_of_mass_symbol_shape (si::Length const radius, ShapeMaterial const& a, ShapeMaterial const& b)
{
	return make_centered_sphere_shape (radius, 8, 8, {   0_deg,  90_deg }, { -90_deg,   0_deg }, a)
		 + make_centered_sphere_shape (radius, 8, 8, {   0_deg,  90_deg }, {   0_deg, +90_deg }, b)
		 + make_centered_sphere_shape (radius, 8, 8, {  90_deg, 180_deg }, { -90_deg,   0_deg }, b)
		 + make_centered_sphere_shape (radius, 8, 8, {  90_deg, 180_deg }, {   0_deg, +90_deg }, a)
		 + make_centered_sphere_shape (radius, 8, 8, { 180_deg, 270_deg }, { -90_deg,   0_deg }, a)
		 + make_centered_sphere_shape (radius, 8, 8, { 180_deg, 270_deg }, {   0_deg, +90_deg }, b)
		 + make_centered_sphere_shape (radius, 8, 8, { 270_deg, 360_deg }, { -90_deg,   0_deg }, b)
		 + make_centered_sphere_shape (radius, 8, 8, { 270_deg, 360_deg }, {   0_deg, +90_deg }, a);
}

} // namespace xf::rigid_body

