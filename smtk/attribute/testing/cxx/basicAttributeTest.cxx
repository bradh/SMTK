/*=========================================================================

Copyright (c) 1998-2012 Kitware Inc. 28 Corporate Drive,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO
PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
=========================================================================*/

#include "smtk/attribute/Manager.h"
#include "smtk/attribute/Definition.h"
#include "smtk/attribute/Attribute.h"
#include "smtk/attribute/StringItemDefinition.h"
#include <iostream>

int main()
{
  int status = 0;
  smtk::attribute::Manager manager;
  std::cout << "Manager Created\n";
  smtk::common::Resource::Type t = manager.resourceType();
  if (t != smtk::common::Resource::ATTRIBUTE)
    {
    std::cout << "ERROR: Returned wrong resource type";
    status++;
    }
  std::cout << "Resource type: "
            << smtk::common::Resource::type2String(t) << "\n";
  smtk::attribute::DefinitionPtr def = manager.createDefinition("testDef");
  if (def)
    {
    std::cout << "Definition testDef created\n";
    }
  else
    {
    std::cout << "ERROR: Definition testDef not created\n";
    status++;
    }
  smtk::attribute::DefinitionPtr def1 = manager.createDefinition("testDef");
  if (!def1)
    {
    std::cout << "Duplicated definition testDef not created\n";
    }
  else
    {
    std::cout << "ERROR: Duplicated definition testDef created\n";
    status++;
    }
  smtk::attribute::AttributePtr att = manager.createAttribute("testAtt", "testDef");
  if (att)
    {
    std::cout << "Attribute testAtt created\n";
    }
  else
    {
    std::cout << "ERROR: Attribute testAtt not created\n";
    status++;
    }

  smtk::attribute::AttributePtr att1 = manager.createAttribute("testAtt", "testDef");
  if (!att1)
    {
    std::cout << "Duplicate Attribute testAtt not created\n";
    }
  else
    {
    std::cout << "ERROR: Duplicate Attribute testAtt  created\n";
    status++;
    }

  std::vector<smtk::attribute::AttributePtr> atts;
  std::vector<smtk::attribute::DefinitionPtr> defs;

  // Check to see how many atts and defs are in the manager
  manager.definitions(defs);
  manager.attributes(atts);

  if (defs.size() != 1)
    {
    std::cout << "Incorrect number of definitions reported - definitions returned: "
              << defs.size() << " but should have returned 1\n";
     status++;
    }
  else if (defs[0] != def)  // Is testDef in the list?
    {
    std::cout << "testDef is not in the list!\n";
    ++status;
    }

  if (atts.size() != 1)
    {
    std::cout << "Incorrect number of attributes reported - attributes returned: "
              << atts.size() << " but should have returned 1\n";
    status++;
    }
  else if (atts[0] != att)  // Is testAtt in the list?
    {
    std::cout << "testAtt is not in the list!\n";
    ++status;
    }


  if (att)
    {
    if( att->id() != 0 )
      {
      std::cout << "Unexpected id: " << att->id() << " should be 0\n";
      status++;
      }
    double color[] = {3,24,12,6};
    if (att->isColorSet())
      {
      std::cout << "Color should not be set.\n";
      status++;
      }
    double const* tcol = att->color();
    if (tcol[0] != 1 || tcol[1] != 1 || tcol[2] != 1 || tcol[3] != 1)
      {
      std::cout << "wrong default color values: " <<tcol[0] << " " << tcol[1] << " " << tcol[2] << ' ' << tcol[3] << std::endl;
      status++;
      }
    att->setColor(color);
    tcol = att->color();
    if (tcol[0] != 3 || tcol[1] != 24 || tcol[2] != 12 || tcol[3] != 6)
      {
      std::cout << "wrong set color values: " <<tcol[0] << " " << tcol[1] << " " << tcol[2] << ' ' << tcol[3] << std::endl;
      status++;
      }
    if (!att->isColorSet())
      {
      std::cout << "Color should be set.\n";
      status++;
      }
    att->unsetColor();
    if (att->isColorSet())
      {
      std::cout << "Color should not be set.\n";
      status++;
      }
    tcol = att->color();
    if (tcol[0] != 1 || tcol[1] != 1 || tcol[2] != 1 || tcol[3] != 1)
      {
      std::cout << "wrong default color values: " <<tcol[0] << " " << tcol[1] << " " << tcol[2] << ' ' << tcol[3] << std::endl;
      status++;
      }
    if ( att->associatedModelEntityIds().size() !=0 )
      {
      std::cout << "Should not have associated entity IDs.\n";
      status++;
      }
    if (att->appliesToBoundaryNodes())
      {
      std::cout << "Should not be applies to boundry node.\n";
      status++;
      }
    att->setAppliesToBoundaryNodes(true);
    if (!att->appliesToBoundaryNodes())
      {
      std::cout << "Should be applies to boundry node.\n";
      status++;
      }
    att->setAppliesToBoundaryNodes(false);
    if (att->appliesToBoundaryNodes())
      {
      std::cout << "Should not be applies to boundry node.\n";
      status++;
      }
    if (att->appliesToInteriorNodes())
      {
      std::cout << "Should not be applies to interior node.\n";
      status++;
      }
    att->setAppliesToInteriorNodes(true);
    if (!att->appliesToInteriorNodes())
      {
      std::cout << "Should be applies to interior node.\n";
      status++;
      }
    att->setAppliesToInteriorNodes(false);
    if (att->appliesToInteriorNodes())
      {
      std::cout << "Should not applies to interior node.\n";
      status++;
      }
    if (att->manager() != &manager)
      {
      std::cout << "Should be this manager.\n";
      status++;
      }
    }

  std::cout << "Manager destroyed\n";
  return status;
}
