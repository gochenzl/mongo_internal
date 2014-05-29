cmake_minimum_required (VERSION 2.6)

set(print_db_ns_SRCS
  mmap_posix.cpp
  util.cpp
  print_db_ns.cpp
  )

set(dump_SRCS
  mmap_posix.cpp
  util.cpp
  datafile_manager.cpp
  dump.cpp
  )
  
set(recover_collection_SRCS
  mmap_posix.cpp
  util.cpp
  datafile_manager.cpp
  recover_collection.cpp
  )

set(CMAKE_CXX_FLAGS "-g -Wall")  
add_executable(print_db_ns ${print_db_ns_SRCS})
add_executable(dump ${dump_SRCS})
add_executable(recover_collection ${recover_collection_SRCS})
