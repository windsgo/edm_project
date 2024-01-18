add_executable(test_spdlog test_spdlog.cpp)
add_dependencies(test_spdlog spdlog)
target_link_libraries(test_spdlog PUBLIC spdlog::spdlog)