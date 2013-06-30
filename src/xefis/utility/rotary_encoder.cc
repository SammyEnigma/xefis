/* vim:ts=4
 *
 * Copyleft 2008…2013  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

// Standard:
#include <cstddef>

// System:
#include <semaphore.h>

// Local:
#include "rotary_encoder.h"


namespace Xefis {

RotaryEncoder::RotaryEncoder (Xefis::PropertyBoolean& property_a, Xefis::PropertyBoolean& property_b, Callback callback):
	_prev_a (property_a.read (false)),
	_prev_b (property_b.read (false)),
	_property_a (property_a),
	_property_b (property_b),
	_callback (callback)
{ }


void
RotaryEncoder::data_updated()
{
	if (!_property_a.valid() || !_property_b.valid() || !_callback)
		return;

	int a = *_property_a;
	int b = *_property_b;
	int da = _prev_a - a;
	int db = _prev_b - b;

	if ((da == 1 && b == 0) || (a == 1 && db == 1) || (da == -1 && b == 1) || (a == 0 && db == -1))
		_callback (Up);
	else
		_callback (Down);
}

} // namespace Xefis

