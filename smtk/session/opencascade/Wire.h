//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================

#ifndef smtk_session_opencascade_Wire_h
#define smtk_session_opencascade_Wire_h
/*!\file */

#include "smtk/graph/Component.h"

#include "smtk/resource/Properties.h"

namespace smtk
{
namespace session
{
namespace opencascade
{

class Resource;

class SMTKOPENCASCADESESSION_EXPORT Wire : public Shape
{
public:
  smtkTypeMacro(Wire);
  smtkSuperclassMacro(smtk::session::opencascade::Shape);
  Wire(const std::shared_ptr<smtk::graph::ResourceBase>& rsrc)
    : Shape(rsrc)
  {
  }
};

} // namespace opencascade
} // namespace session
} // namespace smtk

#endif // smtk_session_opencascade_Wire_h
