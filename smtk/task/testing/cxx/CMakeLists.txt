set(unit_tests
  TestActiveTask.cxx
  TestTaskBasics.cxx
  TestTaskGroup.cxx
  TestTaskJSON.cxx
)

find_package(Threads REQUIRED)

smtk_unit_tests(
  LABEL "Task"
  SOURCES ${unit_tests}
  LIBRARIES smtkCore
)
