include(woincSetupCompilerOptions)

add_executable(run_periodic_tasks run_periodic_tasks.cc)
woincSetupCompilerOptions(run_periodic_tasks)
target_link_libraries(run_periodic_tasks PRIVATE woincui)

