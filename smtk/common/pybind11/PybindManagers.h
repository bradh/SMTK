//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================

#ifndef pybind_smtk_common_Managers_h
#define pybind_smtk_common_Managers_h

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "smtk/common/Managers.h"

#include "smtk/operation/Manager.h"
#include "smtk/resource/Manager.h"
#include "smtk/task/Manager.h"
#include "smtk/view/Manager.h"
#include "smtk/view/Selection.h"

#include <vector>

namespace py = pybind11;

inline PySharedPtrClass< smtk::common::Managers > pybind11_init_smtk_common_Managers(py::module &m)
{
  PySharedPtrClass< smtk::common::Managers > instance(m, "Managers");
  instance
    .def_static("create", (std::shared_ptr<smtk::common::Managers> (*)()) &smtk::common::Managers::create)
    .def("insert_or_assign", [](smtk::common::Managers& managers, std::shared_ptr<smtk::operation::Manager>& operationManager)
      {
        managers.insert_or_assign(operationManager);
      }, py::arg("operationManager"))
    .def("insert_or_assign", [](smtk::common::Managers& managers, std::shared_ptr<smtk::resource::Manager>& resourceManager)
      {
        managers.insert_or_assign(resourceManager);
      }, py::arg("resourceManager"))
    .def("insert_or_assign", [](smtk::common::Managers& managers, std::shared_ptr<smtk::task::Manager>& taskManager)
      {
        managers.insert_or_assign(taskManager);
      }, py::arg("taskManager"))
    .def("insert_or_assign", [](smtk::common::Managers& managers, std::shared_ptr<smtk::view::Manager>& viewManager)
      {
        managers.insert_or_assign(viewManager);
      }, py::arg("viewManager"))
    .def("insert_or_assign", [](smtk::common::Managers& managers, std::shared_ptr<smtk::view::Selection>& viewSelection)
      {
        managers.insert_or_assign(viewSelection);
      }, py::arg("selection"))
    ;
  return instance;
}

#endif
