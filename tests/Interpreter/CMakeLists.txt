add_executable(test_raw_python test_raw_python.cpp)
target_link_libraries(test_raw_python PUBLIC 
    fmt::fmt Python3::Python stdc++fs
)

add_executable(test_boost_python test_boost_python.cpp)
target_link_libraries(test_boost_python PUBLIC 
    fmt::fmt stdc++fs Boost::python Python3::Python
)

add_executable(test_interp_wrapper test_interp_wrapper.cpp)
add_dependencies(test_interp_wrapper edm)
target_include_directories(test_interp_wrapper PUBLIC ${PROJECT_INCLUDE_DIRS})
target_link_libraries(test_interp_wrapper PUBLIC 
    edm fmt::fmt stdc++fs Boost::python Python3::Python
)

