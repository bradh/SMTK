//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================

#ifndef smtk_session_opencascade_Import_h
#define smtk_session_opencascade_Import_h

#include "smtk/common/UUID.h"
#include "smtk/graph/Component.h"
#include "smtk/session/opencascade/Operation.h"

namespace smtk
{
namespace session
{
namespace opencascade
{

class Resource;
class Shape;

/**\brief Import native OpenCASCADE models as well as STEP and IGES data.
  *
  * As with most import operations, this one allows imports into an existing
  * resource or a new one.
  */
class SMTKOPENCASCADESESSION_EXPORT Import : public Operation
{
public:
  smtkTypeMacro(smtk::session::opencascade::Import);
  smtkCreateMacro(Import);
  smtkSharedFromThisMacro(smtk::operation::Operation);
  smtkSuperclassMacro(Operation);

protected:
  /// This method does the bulk of the work importing model data.
  Result operateInternal() override;
  /// Return XML describing the operation inputs and outputs as attributes.
  virtual const char* xmlDescription() const override;
};

} // namespace opencascade
} // namespace session
} // namespace smtk

#endif // smtk_session_opencascade_Import_h
