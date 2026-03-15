cmake_minimum_required(VERSION 2.8.3)
project(annotate_standalone)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(catkin REQUIRED COMPONENTS
  geometry_msgs
  interactive_markers
  roscpp
  rosbag_storage
  rviz
  tf
  tf2_msgs
  visualization_msgs
  std_srvs
  sensor_msgs
  rosgraph_msgs
)

catkin_package(
  INCLUDE_DIRS include
  CATKIN_DEPENDS
    geometry_msgs
    interactive_markers
    roscpp
    rosbag_storage
    rviz
    tf
    tf2_msgs
    visualization_msgs
    std_srvs
    sensor_msgs
    rosgraph_msgs
)

include_directories(
  include
  ${catkin_INCLUDE_DIRS}
  ${rviz_INCLUDE_DIRS}
)

set(CMAKE_AUTOMOC ON)
find_package(Qt5 ${rviz_QT_VERSION} REQUIRED Core Widgets Gui)
set(QT_LIBRARIES Qt5::Widgets Qt5::Gui)
add_definitions(-DQT_NO_KEYWORDS)

## RViz plugin library (loaded by rviz at runtime)
add_library(${PROJECT_NAME}
  src/annotate_display.cpp
  src/annotate_tool.cpp
  src/annotation_marker.cpp
  src/file_dialog_property.cpp
  src/shortcut_property.cpp
  include/${PROJECT_NAME}/annotate_display.h
  include/${PROJECT_NAME}/annotate_tool.h
  include/${PROJECT_NAME}/annotation_marker.h
  include/${PROJECT_NAME}/file_dialog_property.h
  include/${PROJECT_NAME}/shortcut_property.h
)
target_link_libraries(${PROJECT_NAME} ${QT_LIBRARIES} ${catkin_LIBRARIES} yaml-cpp)

## Main standalone application with embedded rviz and bag player
add_executable(${PROJECT_NAME}_node
  src/main.cpp
  src/bag_player.cpp
  src/bag_player_widget.cpp
  include/${PROJECT_NAME}/bag_player.h
  include/${PROJECT_NAME}/bag_player_widget.h
)
target_link_libraries(${PROJECT_NAME}_node ${QT_LIBRARIES} ${catkin_LIBRARIES})

## Install
install(TARGETS ${PROJECT_NAME}
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_GLOBAL_BIN_DESTINATION}
)

install(TARGETS ${PROJECT_NAME}_node
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)

install(DIRECTORY include/${PROJECT_NAME}/
  DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
  FILES_MATCHING PATTERN "*.h"
)

install(FILES plugin_description.xml
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
)

install(DIRECTORY launch/
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}/launch
)
