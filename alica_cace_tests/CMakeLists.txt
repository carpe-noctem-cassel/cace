cmake_minimum_required(VERSION 2.8.3)
project(alica_cace_tests)

## Use c++ 11x std
set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")

## Find catkin macros and libraries
## if COMPONENTS list like find_package(catkin REQUIRED COMPONENTS xyz)
## is used, also find other catkin packages
find_package(catkin REQUIRED COMPONENTS roscpp system_config cace fsystem autodiff constraintsolver event_handling alica_engine alica_test alica_cace_proxy alica_ros_proxy)

## System dependencies are found with CMake's conventions
#find_package(Boost REQUIRED COMPONENTS program_options)



## Uncomment this if the package has a setup.py. This macro ensures
## modules and global scripts declared therein get installed
## See http://ros.org/doc/api/catkin/html/user_guide/setup_dot_py.html
# catkin_python_setup()



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
  INCLUDE_DIRS 
  LIBRARIES alica_engine 
  CATKIN_DEPENDS alica_engine alica_cace_proxy
#  DEPENDS tinyxml
)

###########
## Build ##
###########

file(GLOB_RECURSE test_alica_cace_SOURCES "src/*.cpp")
catkin_add_gtest(${PROJECT_NAME}-test ${test_alica_cace_SOURCES})
if(TARGET ${PROJECT_NAME}-test)
## Specify additional locations of header files
## Your package locations should be listed before other locations
include_directories(include ${catkin_INCLUDE_DIRS})




## Add gtest based cpp test target and link libraries
 
add_dependencies(${PROJECT_NAME}-test ${catkin_LIBRARIES})
target_link_libraries(${PROJECT_NAME}-test ${catkin_LIBRARIES})
endif()
