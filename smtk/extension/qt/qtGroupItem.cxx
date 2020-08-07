//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================

#include "smtk/extension/qt/qtGroupItem.h"

#include "smtk/extension/qt/qtAttribute.h"
#include "smtk/extension/qt/qtBaseAttributeView.h"
#include "smtk/extension/qt/qtTableWidget.h"
#include "smtk/extension/qt/qtUIManager.h"

#include "smtk/attribute/GroupItem.h"
#include "smtk/attribute/GroupItemDefinition.h"
#include "smtk/attribute/ValueItem.h"
#include "smtk/attribute/ValueItemDefinition.h"

#include <QCoreApplication>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QMap>
#include <QPointer>
#include <QScrollBar>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>

using namespace smtk::extension;

class qtGroupItemInternals
{
public:
  QPointer<QFrame> ChildrensFrame;
  QMap<QToolButton*, QList<qtItem*> > ExtensibleMap;
  QList<QToolButton*> MinusButtonIndices;
  QPointer<QToolButton> AddItemButton;
  QPointer<QTableWidget> ItemsTable;
  QPointer<QGroupBox> GroupBox;
  std::map<std::string, qtAttributeItemInfo> m_itemViewMap;
};

qtItem* qtGroupItem::createItemWidget(const qtAttributeItemInfo& info)
{
  // So we support this type of item?
  if (info.itemAs<smtk::attribute::GroupItem>() == nullptr)
  {
    return nullptr;
  }
  return new qtGroupItem(info);
}

qtGroupItem::qtGroupItem(const qtAttributeItemInfo& info)
  : qtItem(info)
{
  m_internals = new qtGroupItemInternals;
  m_itemInfo.createNewDictionary(m_internals->m_itemViewMap);
  m_isLeafItem = true;
  std::string insertMode;
  auto item = m_itemInfo.itemAs<attribute::GroupItem>();

  // We support prepending subgroups iff the group is extensible and
  // the insertion mode is set to prepend
  m_prependMode =
    (item->isExtensible() && m_itemInfo.component().attribute("InsertMode", insertMode) &&
      ((insertMode == "prepend") || (insertMode == "Prepend")));

  this->createWidget();
}

qtGroupItem::~qtGroupItem()
{
  this->clearChildItems();
  delete m_internals;
}

void qtGroupItem::setLabelVisible(bool visible)
{
  auto item = m_itemInfo.itemAs<attribute::GroupItem>();
  if (item == nullptr)
  {
    return;
  }
  if (!item || !item->numberOfGroups())
  {
    return;
  }

  m_internals->GroupBox->setTitle(visible ? item->label().c_str() : "");
}

void qtGroupItem::createWidget()
{
  auto item = m_itemInfo.itemAs<attribute::GroupItem>();
  if (item == nullptr)
  {
    return;
  }
  this->clearChildItems();
  if ((!item->numberOfGroups() && !item->isExtensible()))
  {
    return;
  }

  QString title = item->label().c_str();
  m_internals->GroupBox = new QGroupBox(title, m_itemInfo.parentWidget());
  m_widget = m_internals->GroupBox;

  if (this->isReadOnly())
  {
    m_widget->setEnabled(false);
  }
  // Instantiate a layout for the widget, but do *not* assign it to a variable.
  // because that would cause a compiler warning, since the layout is not
  // explicitly referenced anywhere in this scope. (There is no memory
  // leak because the layout instance is parented by the widget.)
  new QVBoxLayout(m_widget);
  m_widget->layout()->setMargin(0);
  m_internals->ChildrensFrame = new QFrame(m_internals->GroupBox);
  m_internals->ChildrensFrame->setObjectName("groupitemFrame");
  new QVBoxLayout(m_internals->ChildrensFrame);

  m_widget->layout()->addWidget(m_internals->ChildrensFrame);

  if (m_itemInfo.parentWidget())
  {
    m_itemInfo.parentWidget()->layout()->setAlignment(Qt::AlignTop);
    m_itemInfo.parentWidget()->layout()->addWidget(m_widget);
  }
  this->updateItemData();

  // If the group is optional, we need a check box
  if (item->isOptional())
  {
    m_internals->GroupBox->setCheckable(true);
    m_internals->GroupBox->setChecked(item->localEnabledState());
    //Hides empty frame when not enabled.
    m_internals->GroupBox->setStyleSheet("QGroupBox::unchecked {border: none;}");
    m_internals->ChildrensFrame->setVisible(item->isEnabled());
    connect(m_internals->GroupBox, SIGNAL(toggled(bool)), this, SLOT(setEnabledState(bool)));
  }
}

void qtGroupItem::setEnabledState(bool checked)
{
  m_internals->ChildrensFrame->setVisible(checked);
  auto item = m_itemInfo.item();
  if (item == nullptr)
  {
    return;
  }

  if (checked != item->localEnabledState())
  {
    item->setIsEnabled(checked);
    emit this->modified();
    auto iview = m_itemInfo.baseView();
    if (iview)
    {
      iview->valueChanged(item);
    }
  }
}

void qtGroupItem::updateItemData()
{
  // Since an item's optional status can change (using
  // forceRequired) we need to reevaluate the optional status
  auto item = m_itemInfo.itemAs<attribute::GroupItem>();
  if (item->isOptional())
  {
    m_internals->GroupBox->setCheckable(true);
    m_internals->GroupBox->setChecked(item->localEnabledState());
  }
  else
  {
    m_internals->GroupBox->setCheckable(false);
  }
  this->clearChildItems();
  auto myChildren = m_internals->ChildrensFrame->findChildren<QWidget*>("groupitem_frame");
  for (auto myChild : myChildren)
  {
    myChild->deleteLater();
  }

  if (!item || (!item->numberOfGroups() && !item->isExtensible()))
  {
    return;
  }

  std::size_t i, n = item->numberOfGroups();
  if (item->isExtensible())
  {
    //clear mapping
    m_internals->ExtensibleMap.clear();
    m_internals->MinusButtonIndices.clear();
    if (m_internals->ItemsTable)
    {
      m_internals->ItemsTable->blockSignals(true);
      m_internals->ItemsTable->clear();
      m_internals->ItemsTable->setRowCount(0);
      m_internals->ItemsTable->setColumnCount(1);
      m_internals->ItemsTable->setHorizontalHeaderItem(0, new QTableWidgetItem(" "));
      m_internals->ItemsTable->blockSignals(false);
    }

    // The new item button
    if (!m_internals->AddItemButton)
    {
      m_internals->AddItemButton = new QToolButton(m_internals->ChildrensFrame);
      m_internals->AddItemButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
      QString iconName(":/icons/attribute/plus.png");
      std::string extensibleLabel = "Add Row";
      m_itemInfo.component().attribute("ExtensibleLabel", extensibleLabel);
      m_internals->AddItemButton->setText(extensibleLabel.c_str());
      m_internals->AddItemButton->setIcon(QIcon(iconName));
      m_internals->AddItemButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      connect(m_internals->AddItemButton, SIGNAL(clicked()), this, SLOT(onAddSubGroup()));
      m_internals->ChildrensFrame->layout()->addWidget(m_internals->AddItemButton);
    }
    m_widget->layout()->setSpacing(3);
  }

  for (i = 0; i < n; i++)
  {
    int subIdx = static_cast<int>(i);
    if (item->isExtensible())
    {
      this->addItemsToTable(subIdx);
    }
    else
    {
      this->addSubGroup(subIdx);
    }
  }
  this->qtItem::updateItemData();
}

void qtGroupItem::onAddSubGroup()
{
  auto item = m_itemInfo.itemAs<attribute::GroupItem>();
  if (!item || (!item->numberOfGroups() && !item->isExtensible()))
  {
    return;
  }
  if (m_prependMode)
  {
    if (item->prependGroup())
    {
      this->addItemsToTable(0);
      emit this->widgetSizeChanged();
      emit this->modified();
    }
  }
  else
  {
    if (item->appendGroup())
    {
      int subIdx = static_cast<int>(item->numberOfGroups()) - 1;
      if (item->isExtensible())
      {
        this->addItemsToTable(subIdx);
      }
      else
      {
        this->addSubGroup(subIdx);
      }
      emit this->widgetSizeChanged();
      emit this->modified();
    }
  }
}

void qtGroupItem::addSubGroup(int i)
{
  auto item = m_itemInfo.itemAs<attribute::GroupItem>();
  if (!item || (!item->numberOfGroups() && !item->isExtensible()))
  {
    return;
  }
  auto iview = m_itemInfo.baseView();
  if (!iview)
  {
    return;
  }

  const std::size_t numItems = item->numberOfItemsPerGroup();
  QBoxLayout* frameLayout = qobject_cast<QBoxLayout*>(m_internals->ChildrensFrame->layout());
  QFrame* subGroupFrame = new QFrame(m_internals->ChildrensFrame);
  subGroupFrame->setObjectName("groupitemFrame");
  QBoxLayout* subGroupLayout = new QVBoxLayout(subGroupFrame);
  if (item->numberOfGroups() == 1)
  {
    subGroupLayout->setMargin(0);
    subGroupFrame->setFrameStyle(QFrame::NoFrame);
  }
  else
  {
    frameLayout->setMargin(0);
    subGroupFrame->setFrameStyle(QFrame::Panel);
  }
  subGroupLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  QList<qtItem*> itemList;

  auto groupDef = item->definitionAs<attribute::GroupItemDefinition>();
  QString subGroupString;
  if (groupDef->hasSubGroupLabels())
  {
    subGroupString = QString::fromStdString(groupDef->subGroupLabel(i));
    QLabel* subGroupLabel = new QLabel(subGroupString, subGroupFrame);
    subGroupLayout->addWidget(subGroupLabel);
  }

  QList<smtk::attribute::ItemDefinitionPtr> childDefs;
  for (std::size_t j = 0; j < numItems; j++)
  {
    smtk::attribute::ConstItemDefinitionPtr itDef =
      item->item(i, static_cast<int>(j))->definition();
    childDefs.push_back(smtk::const_pointer_cast<attribute::ItemDefinition>(itDef));
  }
  const int tmpLen = m_itemInfo.uiManager()->getWidthOfItemsMaxLabel(
    childDefs, m_itemInfo.uiManager()->advancedFont());
  const int currentLen = iview->fixedLabelWidth();
  iview->setFixedLabelWidth(tmpLen);

  for (std::size_t j = 0; j < numItems; j++)
  {
    auto citem = item->item(i, static_cast<int>(j));
    auto it = m_internals->m_itemViewMap.find(citem->name());
    qtItem* childItem;
    if (it != m_internals->m_itemViewMap.end())
    {
      auto info = it->second;
      info.setParentWidget(m_widget);
      info.setItem(citem);
      childItem = m_itemInfo.uiManager()->createItem(info);
    }
    else
    {
      smtk::view::Configuration::Component comp; // lets create a default style (an empty component)
      qtAttributeItemInfo info(citem, comp, m_widget, m_itemInfo.baseView());
      childItem = m_itemInfo.uiManager()->createItem(info);
    }
    if (childItem)
    {
      this->addChildItem(childItem);
      subGroupLayout->addWidget(childItem->widget());
      itemList.push_back(childItem);
      connect(childItem, SIGNAL(modified()), this, SLOT(onChildItemModified()));
    }
  }
  this->calculateTableHeight();
  iview->setFixedLabelWidth(currentLen);
  frameLayout->addWidget(subGroupFrame);
  this->onChildWidgetSizeChanged();
}

void qtGroupItem::onRemoveSubGroup()
{
  QToolButton* const minusButton = qobject_cast<QToolButton*>(QObject::sender());
  if (!minusButton || !m_internals->ExtensibleMap.contains(minusButton))
  {
    return;
  }

  int gIdx = m_internals->MinusButtonIndices.indexOf(
    minusButton); //minusButton->property("SubgroupIndex").toInt();
  auto item = m_itemInfo.itemAs<attribute::GroupItem>();
  if (!item || gIdx < 0 || gIdx >= static_cast<int>(item->numberOfGroups()))
  {
    return;
  }

  foreach (qtItem* qi, m_internals->ExtensibleMap.value(minusButton))
  {
    // We need to remove the child from our list
    this->removeChildItem(qi);
  }
  //  delete m_internals->ExtensibleMap.value(minusButton).first;
  m_internals->ExtensibleMap.remove(minusButton);

  item->removeGroup(gIdx);
  int rowIdx = -1, rmIdx = -1;
  // normally rowIdx is same as gIdx, but we need to find
  // explicitly since minusButton could be NULL in MinusButtonIndices
  foreach (QToolButton* tb, m_internals->MinusButtonIndices)
  {
    rowIdx = tb != nullptr ? rowIdx + 1 : rowIdx;
    if (tb == minusButton)
    {
      rmIdx = rowIdx;
      break;
    }
  }
  if (rmIdx >= 0 && rmIdx < m_internals->ItemsTable->rowCount())
  {
    m_internals->ItemsTable->removeRow(rmIdx);
  }
  m_internals->MinusButtonIndices.removeOne(minusButton);
  delete minusButton;
  this->calculateTableHeight();
  this->updateExtensibleState();
  emit this->modified();
}

void qtGroupItem::updateExtensibleState()
{
  auto item = m_itemInfo.itemAs<attribute::GroupItem>();
  if (!item || !item->isExtensible())
  {
    return;
  }
  bool maxReached =
    (item->maxNumberOfGroups() > 0) && (item->maxNumberOfGroups() == item->numberOfGroups());
  m_internals->AddItemButton->setEnabled(!maxReached);

  bool minReached = (item->numberOfRequiredGroups() > 0) &&
    (item->numberOfRequiredGroups() == item->numberOfGroups());
  foreach (QToolButton* tButton, m_internals->ExtensibleMap.keys())
  {
    tButton->setEnabled(!minReached);
  }
}

void qtGroupItem::addItemsToTable(int index)
{
  auto item = m_itemInfo.itemAs<attribute::GroupItem>();
  if (!item || !item->isExtensible())
  {
    return;
  }

  std::size_t j, m = item->numberOfItemsPerGroup();
  QBoxLayout* frameLayout = qobject_cast<QBoxLayout*>(m_internals->ChildrensFrame->layout());
  if (!m_internals->ItemsTable)
  {
    m_internals->ItemsTable = new qtTableWidget(m_internals->ChildrensFrame);
    m_internals->ItemsTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_internals->ItemsTable->setColumnCount(1); // for minus button
    m_internals->ItemsTable->setHorizontalHeaderItem(0, new QTableWidgetItem(" "));
    m_internals->ItemsTable->horizontalHeader()->setSectionResizeMode(
      0, QHeaderView::ResizeToContents);
    m_internals->ItemsTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    frameLayout->addWidget(m_internals->ItemsTable);
  }

  m_internals->ItemsTable->blockSignals(true);
  QSizePolicy sizeFixedPolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  QList<qtItem*> itemList;
  int added = 0;
  // We need to determine if the last column needs to stretch to fill extra space.
  // The conditions are:
  // 1. All of the qtItems are of fixedWidth or number of qtItems are 0
  // 2. The last column is not of fixed width - there seems to be a bug in
  // QT when setting the last column to be Interactive - The table does not
  // properly expand to fill the area provided.  Setting the last column to
  // stretch seems to fix this.
  bool stretchLastColumn = true;
  for (j = 0; j < m; j++)
  {
    auto citem = item->item(index, static_cast<int>(j));
    auto it = m_internals->m_itemViewMap.find(citem->name());
    qtItem* childItem;
    if (it != m_internals->m_itemViewMap.end())
    {
      auto info = it->second;
      info.setParentWidget(m_widget);
      info.setItem(citem);
      childItem = m_itemInfo.uiManager()->createItem(info);
    }
    else
    {
      smtk::view::Configuration::Component comp; // lets create a default style (an empty component)
      qtAttributeItemInfo info(citem, comp, m_widget, m_itemInfo.baseView());
      childItem = m_itemInfo.uiManager()->createItem(info);
    }
    if (childItem)
    {
      this->addChildItem(childItem);
      if (added == 0)
      {
        m_internals->ItemsTable->insertRow(index);
      }
      int numCols = m_internals->ItemsTable->columnCount() - 1;
      if (added >= numCols)
      {
        m_internals->ItemsTable->insertColumn(numCols + 1);
        std::string strItemLabel = citem->label().empty() ? citem->name() : citem->label();
        m_internals->ItemsTable->setHorizontalHeaderItem(
          numCols + 1, new QTableWidgetItem(strItemLabel.c_str()));
        if (childItem->isFixedWidth())
        {
          m_internals->ItemsTable->horizontalHeader()->setSectionResizeMode(
            numCols + 1, QHeaderView::ResizeToContents);
        }
        else
        {
          m_internals->ItemsTable->horizontalHeader()->setSectionResizeMode(
            numCols + 1, QHeaderView::Interactive);
          stretchLastColumn = false;
        }
      }
      childItem->setLabelVisible(false);
      m_internals->ItemsTable->setCellWidget(index, added + 1, childItem->widget());
      itemList.push_back(childItem);
      connect(childItem, SIGNAL(widgetSizeChanged()), this, SLOT(onChildWidgetSizeChanged()),
        Qt::QueuedConnection);
      added++;
      connect(childItem, SIGNAL(modified()), this, SLOT(onChildItemModified()));
    }
  }
  // Check to see if the last column is not fixed width and set the  table to stretch
  // the last column if that is the case
  if (!(itemList.isEmpty() || itemList.back()->isFixedWidth()))
  {
    stretchLastColumn = true;
  }
  m_internals->ItemsTable->horizontalHeader()->setStretchLastSection(stretchLastColumn);
  QToolButton* minusButton = nullptr;
  // if there are items
  if (added > 0)
  {
    minusButton = new QToolButton(m_internals->ChildrensFrame);
    QString iconName(":/icons/attribute/minus.png");
    minusButton->setFixedSize(QSize(16, 16));
    minusButton->setIcon(QIcon(iconName));
    minusButton->setSizePolicy(sizeFixedPolicy);
    minusButton->setToolTip("Remove Row");
    //QVariant vdata(static_cast<int>(i));
    //minusButton->setProperty("SubgroupIndex", vdata);
    connect(minusButton, SIGNAL(clicked()), this, SLOT(onRemoveSubGroup()));
    m_internals->ItemsTable->setCellWidget(index, 0, minusButton);

    m_internals->ExtensibleMap[minusButton] = itemList;
  }
  m_internals->MinusButtonIndices.insert(index, minusButton);
  this->updateExtensibleState();

  this->calculateTableHeight();
  m_internals->ItemsTable->blockSignals(false);
  this->onChildWidgetSizeChanged();
}

void qtGroupItem::onChildWidgetSizeChanged()
{
  if (m_internals->ItemsTable)
  {
    // There seems to be a bug in QT - if you ask the table to
    // resize all of its columns to fit their data, the table may
    // not fill up the space provided.  resizing each column seperately
    // does not seem to have this issue.
    int i, n = m_internals->ItemsTable->columnCount();
    for (i = 0; i < n; i++)
    {
      m_internals->ItemsTable->resizeColumnToContents(i);
    }
    // We need to resize the height of the cell incase the
    // child's height has changed
    m_internals->ItemsTable->resizeRowsToContents();
    emit this->widgetSizeChanged();
  }
}

/* Slot for properly emitting signals when an attribute's item is modified */
void qtGroupItem::onChildItemModified()
{
  emit this->modified();
}

void qtGroupItem::calculateTableHeight()
{
  if (m_internals->ItemsTable == nullptr)
  {
    return;
  }
  int numRows = -1; // Set the height to be the entire table
  m_itemInfo.component().attributeAsInt("MinNumberOfRows", numRows);

  if (numRows == -1)
  {
    numRows = m_internals->ItemsTable->verticalHeader()->count();
  }

  int totalHeight = m_internals->ItemsTable->horizontalScrollBar()->height() +
    m_internals->ItemsTable->horizontalHeader()->height();
  for (int i = 0; i < numRows; i++)
  {
    totalHeight += m_internals->ItemsTable->verticalHeader()->sectionSize(i);
  }
  m_internals->ItemsTable->setMinimumHeight(totalHeight);
}
