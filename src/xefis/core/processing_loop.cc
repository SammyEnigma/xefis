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

// Standard:
#include <cstddef>
#include <functional>

// Xefis:
#include <xefis/config/all.h>
#include <xefis/core/machine.h>
#include <xefis/core/module.h>
#include <xefis/core/xefis.h>
#include <xefis/utility/time_helper.h>

// Local:
#include "processing_loop.h"


namespace xf {

ProcessingLoop::ProcessingLoop (Machine* machine, std::string const& name, Frequency loop_frequency, Logger const& logger):
	Module (std::make_unique<ProcessingLoopIO> (name), name),
	_machine (machine),
	_xefis (machine->xefis()),
	_name (name),
	_loop_period (1.0 / loop_frequency),
	_logger (logger)
{
	_loop_timer = new QTimer (this);
	_loop_timer->setSingleShot (false);
	_loop_timer->setInterval (_loop_period.in<Millisecond>());
	QObject::connect (_loop_timer, &QTimer::timeout, this, &ProcessingLoop::execute_cycle);

	_logger.set_processing_loop (*this);
}


ProcessingLoop::~ProcessingLoop()
{
	if (!_modules_tracker.empty())
		Exception::terminate ("ProcessingLoop destroyed while still having registered modules");
}


void
ProcessingLoop::start()
{
	for (auto* module: _uninitialized_modules)
		ModuleIO::ProcessingLoopAPI (*module->io_base()).verify_settings();

	for (auto* module: _uninitialized_modules)
		module->initialize();

	_uninitialized_modules.clear();
	_loop_timer->start();
}


void
ProcessingLoop::stop()
{
	_loop_timer->stop();
}


void
ProcessingLoop::execute_cycle()
{
	Time t = TimeHelper::now();
	Time dt = t - _previous_timestamp.value_or (t - 0.1_ms); // -0.1_ms to prevent division by zero in modules.
	_current_cycle = Cycle { _next_cycle_number++, t, dt, _logger };

	if (_previous_timestamp)
	{
		Time latency = dt - _loop_period;

		io.latency = latency;
		io.actual_frequency = 1.0 / dt;

		if (dt > 1.1 * _loop_period)
			_logger << boost::format ("Latency! %.0f%% delay.\n") % (dt / _loop_period * 100.0 - 100.0);
	}

	for (auto& module_details: _module_details_list)
		BasicModule::ProcessingLoopAPI (module_details.module()).reset_cache();

	for (auto& module_details: _module_details_list)
	{
		module_details.last_processing_time = TimeHelper::measure ([&] {
			BasicModule::ProcessingLoopAPI (module_details.module()).fetch_and_process (*_current_cycle);
		});
	}

	_previous_timestamp = t;
	_current_cycle.reset();
}

} // namespace xf

