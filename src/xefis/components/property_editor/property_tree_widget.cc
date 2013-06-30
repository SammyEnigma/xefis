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
#include <stdexcept>
#include <set>

// Qt:
#include <QtCore/QTimer>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QHeaderView>

// Xefis:
#include <xefis/config/all.h>
#include <xefis/application/services.h>
#include <xefis/core/property_node.h>

// Local:
#include "property_tree_widget.h"
#include "property_tree_widget_item.h"


namespace Xefis {

PropertyTreeWidget::PropertyTreeWidget (PropertyNode* root_node, QWidget* parent):
	QTreeWidget (parent),
	_root_node (root_node)
{
	header()->setSectionsClickable (true);
	header()->setMinimumSectionSize (12.f * Services::default_font_size (physicalDpiY()));
	sortByColumn (NameColumn, Qt::AscendingOrder);
	setSortingEnabled (true);
	setSelectionMode (QTreeWidget::SingleSelection);
	setRootIsDecorated (true);
	setAllColumnsShowFocus (true);
	setAcceptDrops (false);
	setAutoScroll (true);
	setSizePolicy (QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	setVerticalScrollMode (QAbstractItemView::ScrollPerPixel);
	setContextMenuPolicy (Qt::CustomContextMenu);
	setHeaderLabels ({ "Property", "Value" });

	addTopLevelItem (new PropertyTreeWidgetItem (root_node, this));

	read();
	setup_appereance();

	QTimer* timer = new QTimer (this);
	timer->setInterval (1000.0 / 15.0);
	QObject::connect (timer, SIGNAL (timeout()), this, SLOT (read()));
	timer->start();
}


PropertyNode*
PropertyTreeWidget::selected_property_node() const
{
	QList<QTreeWidgetItem*> list = selectedItems();
	if (list.empty())
		return nullptr;
	PropertyTreeWidgetItem* item = dynamic_cast<PropertyTreeWidgetItem*> (list.front());
	if (!item)
		return nullptr;
	return item->node();
}


void
PropertyTreeWidget::read()
{
	read (invisibleRootItem(), _root_node);
}


void
PropertyTreeWidget::read (QTreeWidgetItem* item, PropertyNode* node)
{
	TypedPropertyValueNode* val_node = dynamic_cast<TypedPropertyValueNode*> (node);

	if (val_node)
		item->setData (ValueColumn, Qt::DisplayRole, val_node->is_nil() ? "<nil>" : QString::fromStdString (val_node->stringify()));
	else
	{
		PropertyDirectoryNode* dir_node = dynamic_cast<PropertyDirectoryNode*> (node);

		if (dir_node)
		{
			PropertyNodeList subnodes_list = dir_node->children();
			std::set<PropertyNode*> subnodes (subnodes_list.begin(), subnodes_list.end());

			for (int ci = 0; ci < item->childCount(); ++ci)
			{
				PropertyTreeWidgetItem* c = convert_item (item->child (ci));
				auto s = subnodes.find (c->node());

				if (s != subnodes.end())
				{
					c->read();
					subnodes.erase (s);
				}
				else
					delete item->takeChild (ci--);
			}

			for (auto s: subnodes)
				item->addChild (new PropertyTreeWidgetItem (s, item));
		}
	}
}


PropertyTreeWidgetItem*
PropertyTreeWidget::convert_item (QTreeWidgetItem* item)
{
	PropertyTreeWidgetItem* ret = dynamic_cast<PropertyTreeWidgetItem*> (item);
	if (ret)
		return ret;
	throw std::logic_error ("generic QTreeWidgetItem in PropertyTreeWidget");
}


void
PropertyTreeWidget::setup_appereance()
{
	header()->resizeSection (NameColumn, 20.f * Services::default_font_size (physicalDpiY()));
}

} // namespace Xefis

