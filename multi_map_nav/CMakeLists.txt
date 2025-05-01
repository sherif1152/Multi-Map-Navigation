cmake_minimum_required(VERSION 3.0.2)
project(multi_map_nav)

find_package(catkin REQUIRED COMPONENTS
  roscpp
  std_msgs
  actionlib
  actionlib_msgs
  move_base_msgs
  message_generation
  rospack
  roslib
)

find_package(SQLite3 REQUIRED)

add_action_files(
  DIRECTORY action
  FILES NavigateToGoal.action
)

generate_messages(
  DEPENDENCIES actionlib_msgs std_msgs
)

catkin_package(
  CATKIN_DEPENDS roscpp std_msgs actionlib actionlib_msgs move_base_msgs message_runtime roslib
)

include_directories(
  include
  ${catkin_INCLUDE_DIRS}
  ${SQLite3_INCLUDE_DIRS}
)

add_executable(navigation_server src/navigation_server.cpp src/wormhole_manager.cpp src/map_switcher.cpp)

add_dependencies(navigation_server ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})

target_link_libraries(navigation_server ${catkin_LIBRARIES} sqlite3 )
