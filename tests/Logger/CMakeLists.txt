add_executable(test_spdlog test_spdlog.cpp)
add_dependencies(test_spdlog spdlog)
target_link_libraries(test_spdlog PUBLIC spdlog::spdlog)

add_executable(test_serialize test_serialize.cpp)
add_dependencies(test_serialize spdlog edm)
target_link_libraries(test_serialize PUBLIC fmt::fmt spdlog::spdlog edm)