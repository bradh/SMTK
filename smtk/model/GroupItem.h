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
// .NAME GroupItem.h -
// .SECTION Description
//  Now the attribute association is only linked with group (bc group or domain set)
//  with an id and type, whose enumeration is defined in model::Item.h file.
//  The entityMask variable in this class will be matching
//  with the associationMask inside attribute::Definition.h.
// .SECTION See Also

#ifndef __smtk_model_GroupItem_h
#define __smtk_model_GroupItem_h

#include "smtk/SMTKCoreExports.h"
#include "smtk/PublicPointerDefs.h"
#include "smtk/model/Item.h"
#include <map>
#include <string>

namespace smtk
{
  namespace model
  {
    class SMTKCORE_EXPORT GroupItem : public Item
    {
    public:
      GroupItem(Model *model, int myid, MaskType mask);
      virtual ~GroupItem();
      virtual Item::Type type() const;

      bool canContain(const smtk::model::ItemPtr ptr) const
      {return this->canContain(ptr->type());}
      bool canContain(smtk::model::Item::Type enType) const
      {return ((this->m_entityMask & enType) != 0);}
      virtual std::size_t numberOfItems() const {return this->m_items.size();}
      virtual smtk::model::ItemPtr item(int i) const;
      virtual bool insert(smtk::model::ItemPtr ptr);
      virtual bool remove(smtk::model::ItemPtr ptr);

      MaskType entityMask() const
      { return this->m_entityMask;}

    protected:
      MaskType m_entityMask;
      mutable std::map<int, smtk::model::ItemPtr> m_items;

    private:
    };
  };
};

#endif /* __smtk_model_GroupItem_h */
