//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================

#include "smtk/bridge/polygon/qt/pqSplitEdgeWidget.h"

#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqRenderView.h"
#include "pqRenderViewSelectionReaction.h"

#include "vtkClientServerStream.h"
#include "vtkDoubleArray.h"
#include "vtkNew.h"
#include "vtkPVSelectionInformation.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMVectorProperty.h"
#include "vtkUnsignedIntArray.h"
#include "vtkPolygonArcInfo.h"

#include "smtk/model/Edge.h"
#include "smtk/model/Manager.h"
#include "smtk/model/Operator.h"
#include "smtk/attribute/Attribute.h"
#include "smtk/attribute/DoubleItem.h"

#include <QToolButton>
#include <QVBoxLayout>
#include <QtDebug>

namespace pqSplitEdgeWidgetInternals
{
EdgePointPicker::EdgePointPicker(QObject* p) :
  QAction(p), Selecter(NULL), InteractiveSelectButton(NULL), m_isActive(false)
{ 
  this->setCheckable(true);
}

EdgePointPicker::~EdgePointPicker()
{
  if(this->Selecter)
    {
    this->Selecter->disconnect();
    delete this->Selecter;
    }
}

void EdgePointPicker::doPick(pqRenderView* view)
{
  if(this->Selecter)
    {
    delete this->Selecter;
    }
  this->Selecter = new pqRenderViewSelectionReaction(this, view,
                        pqRenderViewSelectionReaction::SELECT_SURFACE_POINTS_INTERACTIVELY);

  // we only want selection on one representation.
  view->setUseMultipleRepresentationSelection(false);

  this->m_isActive = true;
  emit triggered(true);
}

void EdgePointPicker::donePicking(pqRenderView* view)
{
  //we want the connection to happen once the view goes away so
  //remove the connection
  if(this->Selecter)
    {
    this->Selecter->disconnect();
    delete this->Selecter;
    this->Selecter = NULL;
    }
  if(view)
    {
    view->forceRender();
    // reset multiple selection to true
    view->setUseMultipleRepresentationSelection(true);
    view->setCursor(QCursor());
    }
  //resets the widget to what it would be like if it was just created
  this->InteractiveSelectButton->blockSignals(true);
  this->InteractiveSelectButton->setChecked(false);
  this->InteractiveSelectButton->blockSignals(false);
  this->m_isActive = false;
  emit triggered(false);
}
}

//-----------------------------------------------------------------------------
pqSplitEdgeWidget::pqSplitEdgeWidget(QWidget *prent) :
  QWidget(prent),
  m_edgePointPicker(new pqSplitEdgeWidgetInternals::EdgePointPicker(this)),
  View(NULL)
{
  this->setObjectName("pqSplitEdgeWidget");
  QSizePolicy sizeFixedPolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  QToolButton* splitButton = new QToolButton(this);
  splitButton->setText("Split Edge");
  splitButton->setToolTip("Split an edge by clicking on an edge point");
  splitButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  QString iconName(":/icons/attribute/edgesplit.png");
  splitButton->setIcon(QIcon(iconName));
  splitButton->setIconSize(QSize(32, 32));
  splitButton->setSizePolicy(sizeFixedPolicy);
  splitButton->setCheckable(true);

  this->m_edgePointPicker->InteractiveSelectButton = splitButton;
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setMargin(0);
  layout->addWidget(splitButton);

  //connect up the split buttons
  QObject::connect(splitButton, SIGNAL(toggled(bool)),
    this, SLOT(splitEdgeOperation(bool)));
}

//-----------------------------------------------------------------------------
pqSplitEdgeWidget::~pqSplitEdgeWidget()
{
  delete this->m_edgePointPicker;
}

void pqSplitEdgeWidget::setEdgeOperator(smtk::model::OperatorPtr edgeOp)
{
  if(edgeOp && edgeOp->name() == "split edge")
    this->m_edgeOp = edgeOp;
  else
    this->m_edgeOp = smtk::model::Operator::Ptr();
}

//-----------------------------------------------------------------------------
void pqSplitEdgeWidget::splitEdgeOperation(bool start)
{
  if(this->View && this->m_edgeOp.lock() && start)
    {
    // things are selected
    QObject::connect(this->View,SIGNAL(selected(pqOutputPort*)),
                     this,SLOT(arcPointPicked(pqOutputPort*)),
                     Qt::UniqueConnection);
    // selection is done
    QObject::connect(this->View,SIGNAL(selectionModeChanged(bool)),
                     this,SLOT(resetWidget()),
                     Qt::UniqueConnection);

    this->m_edgePointPicker->doPick(this->View);
    }
  else
    {
    this->resetWidget();
    }
}

void pqSplitEdgeWidget::arcPointPicked(pqOutputPort* port)
{
  if(!this->m_edgePointPicker->m_isActive)
    return;

  if(port && this->View && this->m_edgeOp.lock())
    {
    // get the selected point id
    // This "IDs" only have three components [composite_index, processId, Index]
    // because the arc source is a block in a Composite Dataset
    vtkSMSourceProxy* selSource = port->getSelectionInput();
    // [composite_index, process_id, index]
    vtkSMPropertyHelper selIDs(selSource, "IDs");
    unsigned int count = selIDs.GetNumberOfElements();
    bool readytoOp = false;
    if(count > 2)
      {
      // get first selected point
      vtkIdType flatIdx = selIDs.GetAsInt(0);
      vtkIdType ptid = selIDs.GetAsInt(2);
      vtkNew<vtkPolygonArcInfo> arcInfo;
      //collect the information from the server model source
      vtkSMProxy *proxy = port->getSource()->getProxy();
      arcInfo->SetBlockIndex(flatIdx - 1 );
      arcInfo->SetSelectedPointId(ptid);
      proxy->GatherInformation(arcInfo.GetPointer());
      if(arcInfo->GetModelEntityID())
        {
        smtk::common::UUID edgeId(arcInfo->GetModelEntityID());
        smtk::model::Edge edge(this->m_edgeOp.lock()->manager(), edgeId);
        if(edge.isValid())
          {
          smtk::attribute::AttributePtr opSpec = this->m_edgeOp.lock()->specification();
          opSpec->associateEntity(edge);
          double ptcoords[3];
          arcInfo->GetSelectedPointCoordinates(ptcoords);
          opSpec->findDouble("point")->setValue(0, ptcoords[0]);
          opSpec->findDouble("point")->setValue(1, ptcoords[1]);
          // now request the operation
          readytoOp = true;
          }
        }
      }

    // once find the edge needed, turn off the selection for model source
    port->setSelectionInput(NULL, 0);
    this->View->render();
    if(readytoOp)
      {
      emit this->operationRequested(this->m_edgeOp.lock());
      }
    }
}
//-----------------------------------------------------------------------------
bool pqSplitEdgeWidget::isActive()
{
  return this->m_edgePointPicker->m_isActive;
}
//-----------------------------------------------------------------------------
void pqSplitEdgeWidget::resetWidget()
{
  this->m_edgePointPicker->donePicking(this->View);
}
