#include "ListWidgetEx.h"
#include "Global.h"
#include <QMenu>

ListWidgetEx::ListWidgetEx(const QString& objectName, const QString& defaultItem, QWidget *parent)
	: QListWidget(parent), m_objectName(objectName), m_defaultItem(defaultItem)
{
	setContextMenuPolicy(Qt::CustomContextMenu);
	setMaximumHeight(ITEMLISTMAXHEIGHT);
	setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
	setEditTriggers(QAbstractItemView::DoubleClicked);
	
	connect(this, &ListWidgetEx::customContextMenuRequested,
		this, &ListWidgetEx::customContextMenuRequested);
}


ListWidgetEx::~ListWidgetEx()
{
}


QStringList ListWidgetEx::getItemsText() const
{
	QStringList ret;
	for (int n = 0; n < count(); n++)
	{
		ret.append(item(n)->text());
	}
	return ret;
}


void ListWidgetEx::customContextMenuRequested(const QPoint& point)
{
	QMenu* menu = new QMenu;
	menu->addAction(tr("Add new %1").arg(m_objectName),	[this]()
	{
		QListWidgetItem* item = new QListWidgetItem(m_defaultItem, this);
		item->setFlags(item->flags().setFlag(Qt::ItemIsEditable, true));
		emit valueChanged();
	});
	menu->addAction(tr("Remove selected %1s").arg(m_objectName), [this]()
	{
		QList<QListWidgetItem*> itemList = selectedItems();
		if (itemList.isEmpty())
			return;

		for (const auto& item : itemList)
		{
			removeItemWidget(item);
			delete item;
		}
		emit valueChanged();
	});
	menu->exec(mapToGlobal(point));
}

