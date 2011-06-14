/*=========================================================================

Copyright (c) 1998-2005 Kitware Inc. 28 Corporate Drive, Suite 204,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced,
distributed,
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
// .NAME vtkCmbModelFaceMeshServer - Mesh representation for a vtkModelFace
// .SECTION Description
// Mesh representation for a vtkModelFace.  The smaller value for
// maximum area and larger value for minimum angle wins for local
// vs. global comparisons.  Absolute values are used for area.

#ifndef __vtkCmbModelFaceMeshServer_h
#define __vtkCmbModelFaceMeshServer_h

#include "vtkCmbModelFaceMesh.h"

class vtkModelFace;
class vtkCmbModelVertexMesh;

class VTK_EXPORT vtkCmbModelFaceMeshServer : public vtkCmbModelFaceMesh
{
public:
  static vtkCmbModelFaceMeshServer* New();
  vtkTypeRevisionMacro(vtkCmbModelFaceMeshServer,vtkCmbModelFaceMesh);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the local mesh length, min angle
  virtual bool SetLocalLength(double length);
  virtual bool SetLocalMinimumAngle(double angle);

protected:
  vtkCmbModelFaceMeshServer();
  virtual ~vtkCmbModelFaceMeshServer();

  // Description:
  // This method builds the model entity's mesh without checking
  // the parameters.  Returns true if the operation succeeded as
  // desired.  This includes deleting the mesh if the mesh
  // parameters go from valid to invalid values (i.e. a parameter set to 0).
  bool BuildMesh(bool meshHigherDimensionalEntities);

  bool CreateMeshInfo();
  bool Triangulate(vtkPolyData *mesh, double length, double angle);
  void DetermineZValueOfFace();

private:

  double ZValue;
  vtkCmbModelFaceMeshServer(const vtkCmbModelFaceMeshServer&);  // Not implemented.
  void operator=(const vtkCmbModelFaceMeshServer&);  // Not implemented.

  //BTX
  CmbModelFaceMeshPrivate::ModelFaceRep *FaceInfo;
  //ETX
};

#endif
