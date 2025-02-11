//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================

#include "smtk/common/UUIDGenerator.h"
#include "smtk/resource/Component.h"
#include "smtk/resource/DerivedFrom.h"
#include "smtk/resource/Manager.h"

#include "smtk/operation/Manager.h"

#include "smtk/project/Manager.h"
#include "smtk/project/Project.h"
#include "smtk/project/Registrar.h"
#include "smtk/project/operators/Define.h"

#include "smtk/plugin/Registry.h"

#include "smtk/common/testing/cxx/helpers.h"

// This test registers a project type ("MyProject") with a resource whitelist with
// one type ("MyResource"). It then creates project and resource instances and adds
// the resource to the project, and verifies that the resource can be accessed from
// the project. It then removes the resource from the project, deletes the project,
// and verifies that the resource is still held in memory by the resource manager.

namespace
{
class MyResource : public smtk::resource::DerivedFrom<MyResource, smtk::resource::Resource>
{
public:
  smtkTypeMacro(MyResource);
  smtkCreateMacro(MyResource);
  smtkSharedFromThisMacro(smtk::resource::PersistentObject);

  smtk::resource::ComponentPtr find(const smtk::common::UUID& /*compId*/) const override
  {
    return smtk::resource::ComponentPtr();
  }

  std::function<bool(const smtk::resource::Component&)> queryOperation(
    const std::string& /*unused*/) const override
  {
    return [](const smtk::resource::Component& /*unused*/) { return true; };
  }

  void visit(smtk::resource::Component::Visitor& /*v*/) const override {}

protected:
  MyResource()
    : smtk::resource::DerivedFrom<MyResource, smtk::resource::Resource>()
  {
  }
};

struct Registrar
{
  static void registerTo(const smtk::resource::Manager::Ptr& resourceManager)
  {
    resourceManager->registerResource<MyResource>();
  }

  static void unregisterFrom(const smtk::resource::Manager::Ptr& resourceManager)
  {
    resourceManager->unregisterResource<MyResource>();
  }

  static void registerTo(const smtk::project::Manager::Ptr& projectManager)
  {
    projectManager->registerProject("MyProject", { "MyResource" }, {});
  }

  static void unregisterFrom(const smtk::project::Manager::Ptr& projectManager)
  {
    (void)projectManager;
    // projectManager->unregisterProject("MyProject");
  }
};
} // namespace

int TestDefineOp(int /*unused*/, char** const /*unused*/)
{
  // Create a resource manager
  smtk::resource::ManagerPtr resourceManager = smtk::resource::Manager::create();

  smtkTest(resourceManager->metadata().empty(), "New resource manager should have no types.");
  smtkTest(resourceManager->empty(), "New resource manager should have no resources.");

  // Register MyResource
  auto registry = smtk::plugin::addToManagers<::Registrar>(resourceManager);
  smtkTest(
    resourceManager->metadata().size() == 1, "Resource manager should have registered a type.");

  // Create an operation manager
  smtk::operation::ManagerPtr operationManager = smtk::operation::Manager::create();

  // Create a project manager
  smtk::project::ManagerPtr projectManager =
    smtk::project::Manager::create(resourceManager, operationManager);

  // Register project operations with the project manager
  auto projectRegistry = smtk::plugin::addToManagers<smtk::project::Registrar>(projectManager);

  auto createProjectTypeOp = operationManager->create<smtk::project::Define>();

  smtkTest(!!createProjectTypeOp, "Could not create \"Define\" operation.");

  smtkTest(projectManager->metadata().empty(), "New project manager should have no types.");
  smtkTest(projectManager->projects().empty(), "New project manager should have no projects.");

  // Registrar our not-so-custom Project with the manager.
  ::Registrar::registerTo(projectManager);
  smtkTest(
    projectManager->metadata().size() == 1, "Project manager should have registered a type.");

  smtk::resource::Resource::Ptr resource;
  {
    // Create an instance of smtk::project::Project
    auto project = projectManager->create("MyProject");
    smtkTest(projectManager->projects().size() == 1, "Project not added to manaager");

    // Create a resource
    auto myResource = resourceManager->create<MyResource>();
    smtkTest(resourceManager->size() == 1, "Resource not added to manaager");

    project->resources().registerResource<MyResource>();

    // Add the resource to the project
    project->resources().add(myResource);

    // Assign our resource shared_ptr to the project's resource (its shared
    // pointer has an alias for removing the resource from management upon
    // deletion)
    resource = project->resources().get(myResource->id());
    smtkTest(!!resource, "could not access resource from project interface");

    // Remove the instance of smtk::project::Project
    projectManager->remove(project);

    // Test that the resource is still managed...
    smtkTest(resourceManager->size() == 1, "project should be gone, resource should have remained");
  }

  // ...irrespective of scope
  smtkTest(resourceManager->size() == 1, "project should be gone, resource should have remained");

  // Copy the resource's address for testing purposes
  void* resource_address = resource.get();

  // Strip the resource's shared_ptr of its container
  resource = resource->shared_from_this();

  // Test that the underlying resource is still the same as before...
  smtkTest(resource.get() == resource_address, "resource address should have remained the same");
  // ...but it is no longer managed by the resource manager.
  smtkTest(resourceManager->empty(), "Destructed project should have removed resource");

  return 0;
}
