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

#ifndef XEFIS__MODULES__SYSTEMS__AFCS_EAC_YD_H__INCLUDED
#define XEFIS__MODULES__SYSTEMS__AFCS_EAC_YD_H__INCLUDED

// Standard:
#include <cstddef>
#include <string>

// Xefis:
#include <xefis/config/all.h>
#include <xefis/core/module.h>
#include <xefis/core/property.h>
#include <xefis/core/property_observer.h>
#include <xefis/core/setting.h>
#include <xefis/utility/pid_control.h>


class AFCS_EAC_YD_IO: public xf::ModuleIO
{
  public:
	/*
	 * Settings
	 */

	xf::Setting<xf::PIDControl<si::Force, si::Angle>::Settings>
								rudder_pid_settings	{ this, "rudder_pid_settings" };
	xf::Setting<double>			rudder_pid_gain		{ this, "rudder_pid_gain", 1.0 };
	xf::Setting<si::Angle>		deflection_limit	{ this, "deflection_limit" };

	/*
	 * Input
	 */

	xf::PropertyIn<bool>		enabled				{ this, "enabled" };
	xf::PropertyIn<si::Force>	slip_skid			{ this, "slip-skid" };

	/*
	 * Output
	 */

	xf::PropertyOut<si::Angle>	rudder_deflection	{ this, "rudder-deflection" };
};


/**
 * Controls rudder to obtain zero slip-skid value.
 */
class AFCS_EAC_YD: public xf::Module<AFCS_EAC_YD_IO>
{
  public:
	// Ctor
	explicit
	AFCS_EAC_YD (std::unique_ptr<AFCS_EAC_YD_IO>, std::string const& instance = {});

  private:
	// Module API
	void
	initialize() override;

	// Module API
	void
	process (xf::Cycle const&) override;

	/**
	 * Compute rudder.
	 */
	void
	compute();

  private:
	xf::PIDControl<si::Force, si::Angle>	_rudder_pid;
	xf::PropertyObserver					_rudder_computer;
};

#endif
