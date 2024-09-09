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

#ifndef XEFIS__SUPPORT__UI__WIDGETS__PANEL_NUMERIC_DISPLAY_H__INCLUDED
#define XEFIS__SUPPORT__UI__WIDGETS__PANEL_NUMERIC_DISPLAY_H__INCLUDED

// Xefis:
#include <xefis/config/all.h>
#include <xefis/core/v1/socket.h>
#include <xefis/support/ui/widgets/panel_widget.h>

// Qt:
#include <QtWidgets/QDial>
#include <QtGui/QPixmap>

// Standard:
#include <cstddef>
#include <format>
#include <array>


namespace xf {

class PanelNumericDisplay: public PanelWidget
{
	static constexpr int BorderWidth	= 2;
	static constexpr int Margin			= 2;

	static constexpr std::size_t MinusSymbolIndex	= 10;
	static constexpr std::size_t EmptySymbolIndex	= 11;
	static constexpr std::size_t DotSymbolIndex		= 12;

  private:
	// Ctor
	explicit
	PanelNumericDisplay (QWidget* parent, Panel*, unsigned int num_digits, std::string unit, ModuleSocketPath const& value_socket_path);

  public:
	/**
	 * Create 7-segment display for floatizable sockets with hard-coded display format.
	 */
	explicit
	PanelNumericDisplay (QWidget* parent, Panel*, unsigned int num_digits, std::string unit, ModuleSocketPath const& value_socket_path, std::string const& format);

	/**
	 * Create 7-segment display for floatizable sockets with display format provided in a socket.
	 */
	explicit
	PanelNumericDisplay (QWidget* parent, Panel*, unsigned int num_digits, std::string unit, ModuleSocketPath const& value_socket_path, Socket<std::string> const& format_socket);

  protected:
	void
	paintEvent (QPaintEvent*) override;

	void
	data_updated() override;

  private slots:
	/**
	 * Read data from socket.
	 */
	void
	read();

  private:
	/**
	 * Convert a float to string of characters to display.
	 * Returned string will always have size equal to num_digits or one character more
	 * (the dot).
	 */
	std::string
	convert_to_digits (double value);

  private:
	unsigned int				_num_digits { 0 };
	std::string					_unit;
	std::format_string<double>	_static_format { "{}" }; // TODO std::variant with _dynamic_format
	Socket<std::string>			_dynamic_format;
	std::vector<QPixmap*>		_digits_to_display;	// Pointers not owned.
	BasicSocket					_value_socket;
	std::array<QPixmap, 13>		_digit_images;		// [10] is minus sign, [11] is empty, [12] is dot.
};

} // namespace xf

#endif

