cmake_minimum_required(VERSION 3.5)
project(teleop_telemetry_receiver)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)

include_directories(include)

add_executable(telemetry_receiver_node
  src/telemetry_receiver_node.cpp
)

ament_target_dependencies(telemetry_receiver_node
  rclcpp
  geometry_msgs
  sensor_msgs
  std_msgs
)

install(TARGETS telemetry_receiver_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include/
)

ament_package()
