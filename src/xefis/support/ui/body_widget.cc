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

// Local:
#include "body_widget.h"

// Xefis:
#include <xefis/config/all.h>
#include <xefis/support/nature/constants.h>

// Neutrino:
#include <neutrino/format.h>

// Qt:
#include <QGridLayout>

// Standard:
#include <cstddef>


namespace xf {

BodyWidget::BodyWidget (rigid_body::Body* body):
	_body (body)
{
	if (_body)
	{
		add_observable ("Mass", [this]() {
			return neutrino::format_unit (_body->mass_moments().mass().in<si::Kilogram>() * 1000.0, 6, "g");
		});
		add_observable ("Translational kinetic energy", [this]() {
			return neutrino::format_unit (_body->translational_kinetic_energy().in<si::Joule>(), 6, "J");
		});
		add_observable ("Rotational kinetic energy", [this]() {
			return neutrino::format_unit (_body->rotational_kinetic_energy().in<si::Joule>(), 6, "J");
		});
		add_observable ("Load factor", [this]() {
			// TODO low pass filter on load factor:
			auto const acceleration = _body->acceleration_moments_except_gravity<rigid_body::BodyCOM>().acceleration();
			// Wing's down in BodyCOM (airfoil coordinates) is negative Y, so use .y():
			return std::format ("{:.2f}", acceleration.y() / xf::kStdGravitationalAcceleration);
		});
	}
}


void
BodyWidget::update_body_values()
{
	for (auto& observable: _observables)
	{
		auto const text = observable.get ? observable.get() : "–";
		observable.value_label->setText (QString::fromStdString (text));

		// TODO handle observable.set
	}
}


void
BodyWidget::add_widget (QWidget& widget)
{
	auto row = _layout.rowCount();
	_layout.addWidget (&widget, row, 0, 1, 2);
}


QLabel&
BodyWidget::add_observable (std::string_view const name, Getter const getter, Setter const setter)
{
	auto* value_label = new QLabel ("–");
	_observables.emplace_back (value_label, getter, setter);

	auto row = _layout.rowCount();
	_layout.addWidget (new QLabel (QString::fromStdString (std::string (name))), row, 0);
	_layout.addWidget (value_label, row, 1);

	return *value_label;
}

QLabel&
BodyWidget::add_observable (std::string_view const name, std::string& observed_string, Setter const setter)
{
	return add_observable (name, [&observed_string]() { return observed_string; }, setter);
}


std::unique_ptr<BodyWidget>
HasBodyWidget::create_body_widget()
{
	return std::make_unique<BodyWidget> (dynamic_cast<rigid_body::Body*> (this));
}

} // namespace xf

