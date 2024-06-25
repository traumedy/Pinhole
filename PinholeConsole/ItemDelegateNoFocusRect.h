#pragma once

#include <QItemDelegate>

// Allows QTableView (and other table/list views/widgets) to not have a focus rectangle

class ItemDelegateNoFocusRect : public QItemDelegate
{
	Q_OBJECT
public:
	virtual void drawFocus(QPainter* /*painter*/, const QStyleOptionViewItem& /*option*/, const QRect& /*rect*/) const override {};
};
