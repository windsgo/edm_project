add_executable(test_json test_json.cpp)
add_dependencies(test_json edm)
target_link_libraries(test_json edm)

