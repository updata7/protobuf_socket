cmake_minimum_required(VERSION 2.6)

add_custom_command(OUTPUT msg.pb.cc resolver.pb.h
  COMMAND protoc
  ARGS --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/msg.proto -I${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS msg.proto)

project(CHENK)

add_library(msg_proto msg.pb.cc)
target_link_libraries(msg_proto protobuf pthread)

set(CLIENT ../client.cc ../dealpack.cc)
add_executable(client ${CLIENT})
target_link_libraries(client msg_proto protobuf)

set(SERVER ../server.cc ../dealpack.cc)
add_executable(server ${SERVER})
target_link_libraries(server msg_proto protobuf)

set(CONTEST ../contest.cc ../dealpack.cc)
add_executable(contest ${CONTEST})
target_link_libraries(contest msg_proto protobuf)
