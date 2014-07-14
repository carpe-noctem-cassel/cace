cmake_minimum_required(VERSION 2.8.3)
project(cace)

## Use c++ 11x std
set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")

## Find catkin macros and libraries
## if COMPONENTS list like find_package(catkin REQUIRED COMPONENTS xyz)
## is used, also find other catkin packages
find_package(catkin REQUIRED COMPONENTS roscpp system_config message_generation)


## System dependencies are found with CMake's conventions
find_package(Boost REQUIRED COMPONENTS system)


## Uncomment this if the package has a setup.py. This macro ensures
## modules and global scripts declared therein get installed
## See http://ros.org/doc/api/catkin/html/user_guide/setup_dot_py.html
# catkin_python_setup()

################################################
## Declare ROS messages, services and actions ##
################################################

## To declare and build messages, services or actions from within this
## package, follow these steps:
## * Let MSG_DEP_SET be the set of packages whose message types you use in
##   your messages/services/actions (e.g. std_msgs, actionlib_msgs, ...).
## * In the file package.xml:
##   * add a build_depend and a run_depend tag for each package in MSG_DEP_SET
##   * If MSG_DEP_SET isn't empty the following dependencies might have been
##     pulled in transitively but can be declared for certainty nonetheless:
##     * add a build_depend tag for "message_generation"
##     * add a run_depend tag for "message_runtime"
## * In this file (CMakeLists.txt):
##   * add "message_generation" and every package in MSG_DEP_SET to
##     find_package(catkin REQUIRED COMPONENTS ...)
##   * add "message_runtime" and every package in MSG_DEP_SET to
##     catkin_package(CATKIN_DEPENDS ...)
##   * uncomment the add_*_files sections below as needed
##     and list every .msg/.srv/.action file to be processed
##   * uncomment the generate_messages entry below
##   * add every package in MSG_DEP_SET to generate_messages(DEPENDENCIES ...)

## Generate messages in the 'msg' folder
 add_message_files(
   DIRECTORY msg FILES
   CaceAcknowledge.msg
   CaceBelieveNotification.msg
   CaceCommand.msg
   CaceShortAck.msg
   CaceTime.msg
   CaceType.msg
   CaceVariableRequest.msg
)

## Generate services in the 'srv' folder
# add_service_files(
#   FILES
#   Service1.srv
#   Service2.srv
# )

## Generate actions in the 'action' folder
# add_action_files(
#   FILES
#   Action1.action
#   Action2.action
# )

## Generate added messages and services with any dependencies listed here
 generate_messages(
   DEPENDENCIES
   std_msgs  # Or other packages containing msgs
 )

###################################
## catkin specific configuration ##
###################################
## The catkin_package macro generates cmake config files for your package
## Declare things to be passed to dependent projects
## INCLUDE_DIRS: uncomment this if you package contains header files
## LIBRARIES: libraries you create in this project that dependent projects also need
## CATKIN_DEPENDS: catkin_packages dependent projects also need
## DEPENDS: system dependencies of this project that dependent projects also need
catkin_package(
  INCLUDE_DIRS include ${catkin_INCLUDE_DIRS}
  LIBRARIES cace system_config
  CATKIN_DEPENDS roscpp message_runtime 
  DEPENDS program_options 
)

###########
## Build ##
###########

## Specify additional locations of header files
## Your package locations should be listed before other locations
 include_directories(include ${catkin_INCLUDE_DIRS})

file (GLOB _my_sources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/communication/*.cpp src/communication/jobs/*.cpp src/communication/multicast/*.cpp src/timeManager/*.cpp src/variableStore/*.cpp src/variables/*.cpp src/*.cpp)

## Declare a cpp library
 add_library(${PROJECT_NAME}
   ${_my_sources}
 )
#set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-ftest-coverage -fprofile-arcs --coverage ")
#set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-ftest-coverage -fprofile-arcs --coverage ")
 

## Add cmake target dependencies of the executable/library
## as an example, message headers may need to be generated before nodes
# add_dependencies(alica_engine_node alica_engine_generate_messages_cpp)

add_dependencies(${PROJECT_NAME} ${catkin_LIBRARIES})

## Specify libraries to link a library or executable target against
 target_link_libraries(${PROJECT_NAME}
   ${catkin_LIBRARIES}
 )
#set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-DUSE_ROS")

 ## Declare a cpp executable
add_executable(caceMonitor src/caceMonitor/caceMonitor.cpp)
add_dependencies(caceMonitor ${catkin_LIBRARIES})
target_link_libraries(caceMonitor ${catkin_LIBRARIES} ${PROJECT_NAME})

#############
## Testing ##
#############

## Add gtest based cpp test target and link libraries
catkin_add_gtest(${PROJECT_NAME}-test ${_my_sources} test/caceTests.cpp test/jobTests.cpp test/systemTests.cpp test/applicationTests.cpp test/serialization.cpp test/multicasttests.cpp)

if(TARGET ${PROJECT_NAME}-test)
target_link_libraries(${PROJECT_NAME}-test ${catkin_LIBRARIES})
set_target_properties(${PROJECT_NAME}-test PROPERTIES LINK_FLAGS "-ftest-coverage -fprofile-arcs --coverage -g")
set_target_properties(${PROJECT_NAME}-test PROPERTIES COMPILE_FLAGS "-ftest-coverage -fprofile-arcs --coverage -g")
#set_target_properties(${PROJECT_NAME}-test PROPERTIES COMPILE_FLAGS "-ftest-coverage -fprofile-arcs --coverage -g -DUSE_ROS")


catkin_add_gtest(${PROJECT_NAME}-eval ${_my_sources} test/eval/speed.cpp)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}-eval ${catkin_LIBRARIES} ${Boost_LIBRARIES})
set_target_properties(${PROJECT_NAME}-eval PROPERTIES LINK_FLAGS "-O5")
set_target_properties(${PROJECT_NAME}-eval PROPERTIES COMPILE_FLAGS "-O5  -mtune=native -lrt -ftree-vectorize -msse3 -ffast-math -O3 -funroll-loops -fomit-frame-pointer")
#set_target_properties(${PROJECT_NAME}-eval PROPERTIES COMPILE_FLAGS "-O5 -DUSE_ROS")


catkin_add_gtest(${PROJECT_NAME}-timeeval ${_my_sources} test/eval/timeEval.cpp)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}-timeeval ${catkin_LIBRARIES} ${Boost_LIBRARIES})
set_target_properties(${PROJECT_NAME}-timeeval PROPERTIES LINK_FLAGS "-O5")
set_target_properties(${PROJECT_NAME}-timeeval PROPERTIES COMPILE_FLAGS "-O5  -mtune=native -lrt -ftree-vectorize -msse3 -ffast-math -O3 -funroll-loops -fomit-frame-pointer")
#set_target_properties(${PROJECT_NAME}-timeeval PROPERTIES COMPILE_FLAGS "-O5 -DUSE_ROS")


endif()
# catkin_add_nosetests(test)
