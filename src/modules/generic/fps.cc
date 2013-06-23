/* vim:ts=4
 *
 * Copyleft 2012…2013  Michał Gawron
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

// Xefis:
#include <xefis/config/all.h>

// Local:
#include "fps.h"


XEFIS_REGISTER_MODULE_CLASS ("generic/fps", FPS);


FPS::FPS (Xefis::ModuleManager* module_manager, QDomElement const&):
	Module (module_manager),
	_smoother (5_ms)
{ }


void
FPS::data_updated()
{
	fprintf (stderr, "FPS: %.2f\n", _smoother.process (1.f / update_dt().s(), update_dt()));
}

