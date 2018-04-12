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

#ifndef XEFIS__CORE__COMPONENTS__CONFIGURATOR__CONFIGURATOR_WIDGET_H__INCLUDED
#define XEFIS__CORE__COMPONENTS__CONFIGURATOR__CONFIGURATOR_WIDGET_H__INCLUDED

// Standard:
#include <cstddef>

// Qt:
#include <QtWidgets/QWidget>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QLabel>

// Xefis:
#include <xefis/config/all.h>
#include <xefis/core/components/data_recorder/data_recorder.h>
#include <xefis/core/v2/module.h>

// Local:
#include "modules_list.h"


namespace xf {

class ConfiguratorWidget: public QWidget
{
	Q_OBJECT

	/**
	 * Configuration widget for module.
	 * Contains generic config widgets, module's configurator widget,
	 * and other stuff.
	 */
	class GeneralModuleWidget: public QWidget
	{
	  public:
		// Ctor
		explicit
		GeneralModuleWidget (xf::BasicModule&, ConfiguratorWidget&, QWidget* parent);

		/**
		 * Return module.
		 */
		xf::BasicModule&
		module() const noexcept;

	  private:
		xf::BasicModule&	_module;
		ConfiguratorWidget&	_configurator_widget;
	};

  public:
	// Ctor
	explicit
	ConfiguratorWidget (xf::Machine& machine, QWidget* parent);

  private:
	void
	module_selected (xf::BasicModule&);

	void
	none_selected();

	/**
	 * Causes module widget to be reloaded.
	 * The one passed in parameter will be deleted.
	 */
	void
	reload_module_widget (GeneralModuleWidget*);

  private:
	xf::Machine&		_machine;
	ModulesList*		_modules_list			= nullptr;
	QStackedWidget*		_modules_stack			= nullptr;
	DataRecorder*		_data_recorder			= nullptr;
	QTabWidget*			_tabs					= nullptr;
	QLabel*				_no_module_selected		= nullptr;
	std::map<xf::BasicModule*, Shared<GeneralModuleWidget>>
						_general_module_widgets;
};


inline xf::BasicModule&
ConfiguratorWidget::GeneralModuleWidget::module() const noexcept
{
	return _module;
}

} // namespace xf

#endif

