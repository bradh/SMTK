//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================
/*! \file */
#ifndef smtk_task_Instances_h
#define smtk_task_Instances_h

#include "smtk/common/Instances.h"
#include "smtk/common/Managers.h"
#include "smtk/common/TypeName.h"

#include "smtk/task/Task.h"

namespace smtk
{
namespace task
{

/// An enum for events in the lifecycle of a workflow (tree of tasks).
enum class WorkflowEvent
{
  Created,     //!< A workflow has been created.
  TaskAdded,   //!< A task has been added to the workflow.
  TaskRemoved, //!< A task has been removed from the workflow.
  Destroyed, //!< The tasks in the workflow have become unmanaged or dependent on another workflow.
  Resuming   //!< Notifications had been paused and now are not.
             //!< The reported workflow heads are all extant workflows current.
  //!< This event is also used to initialize observers added after tasks have been created.
};

/// Track smtk::task::Task objects with smtk::common::Instances.
///
/// This class adds an API for observing vectors of tasks that
/// form workflows as instances are managed/unmanaged and
/// dependencies are added/removed.
class SMTKCORE_EXPORT Instances
  : public smtk::common::Instances<
      smtk::task::Task,
      void,
      std::tuple<smtk::task::Task::Configuration&, std::shared_ptr<smtk::common::Managers>>,
      std::tuple<
        smtk::task::Task::Configuration&,
        smtk::task::Task::PassedDependencies,
        std::shared_ptr<smtk::common::Managers>>>
{
public:
  /// The signature for observing workflow construction/destruction/modification.
  ///
  /// The first parameter is the head task of the workflow (i.e., the one on which
  /// all others depend). The second parameter is the type of event and the third
  /// parameter is a task that is non-null only for WorkflowEvent::TaskAdded and
  /// WorkflowEvent::TaskRemoved events:
  /// + For WorkflowEvent::TaskAdded, the third parameter is the added task.
  /// + For WorkflowEvent::TaskRemoved, the third parameter is the removed task.
  using WorkflowObserver = std::function<void(const std::set<Task*>&, WorkflowEvent, Task*)>;
  /// Observers that are invoked when the workflow structure changes.
  using WorkflowObservers = smtk::common::Observers<WorkflowObserver>;

  Instances();

  /// Return the set of workflow-event observers (so you can add yourself to it).
  const WorkflowObservers& workflowObservers() const { return m_workflowObservers; }
  WorkflowObservers& workflowObservers() { return m_workflowObservers; }

  /// Pause or resume workflow-event notifications.
  ///
  /// This exists so that deserialization does not generate many
  /// redundant events (e.g., creating a new head for each task
  /// then destroying each as dependencies are added).
  bool pauseWorkflowNotifications(bool doPause);

  /// Notify observers of a change.
  /// Use this method to invoke observers since it can be "paused"
  /// (i.e., no observers will be invoked if \a m_workflowNotificationsPaused
  /// is true).
  bool workflowEvent(const std::set<Task*>& workflows, WorkflowEvent event, Task* subject);

  // Returns the tasks with the given title
  std::set<smtk::task::Task::Ptr> findByTitle(const std::string& title) const;

protected:
  WorkflowObservers m_workflowObservers;
  bool m_workflowNotificationsPaused = false;
  bool m_needNotification = false;
};

} // namespace task
} // namespace smtk

#endif // smtk_task_Instances_h
