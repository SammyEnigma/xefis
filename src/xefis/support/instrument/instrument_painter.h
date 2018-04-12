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

#ifndef XEFIS__SUPPORT__INSTRUMENT__INSTRUMENT_PAINTER_H__INCLUDED
#define XEFIS__SUPPORT__INSTRUMENT__INSTRUMENT_PAINTER_H__INCLUDED

// Standard:
#include <cstddef>

// Qt:
#include <QtGui/QPainter>

// Xefis:
#include <xefis/config/all.h>
#include <xefis/support/instrument/shadow_painter.h>
#include <xefis/support/instrument/text_painter.h>


namespace xf {

class InstrumentPainter:
	virtual public QPainter,
	public TextPainter,
	public ShadowPainter
{
  public:
	// Ctor
	explicit
	InstrumentPainter (QPaintDevice&, TextPainter::Cache&);
};

} // namespace xf

#endif

