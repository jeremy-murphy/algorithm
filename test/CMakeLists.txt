set(CMAKE_CXX_STANDARD 11)

set(Boost_NO_SYSTEM_PATHS ON)
set(BOOST_ROOT "~/projects/boost")
find_package(Boost REQUIRED COMPONENTS chrono filesystem system timer unit_test_framework)

add_executable(search_test1 search_test1.cpp)
target_compile_definitions(search_test1 PRIVATE "BOOST_TEST_DYN_LINK=1")
target_link_libraries(search_test1 Boost::headers Boost::unit_test_framework)
add_test(NAME search_test1 COMMAND search_test1 WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(search_test2 search_test2.cpp)
target_compile_definitions(search_test2 PRIVATE "BOOST_TEST_DYN_LINK=1")
target_link_libraries(search_test2 Boost::chrono Boost::filesystem Boost::system Boost::headers Boost::unit_test_framework)

add_executable(search_test3 search_test3.cpp)
target_compile_definitions(search_test3 PRIVATE "BOOST_TEST_DYN_LINK=1")
target_link_libraries(search_test3 Boost::chrono Boost::filesystem Boost::system Boost::headers Boost::unit_test_framework)

add_executable(search_test4 search_test4.cpp)
target_compile_definitions(search_test4 PRIVATE "BOOST_TEST_DYN_LINK=1")
target_link_libraries(search_test4 Boost::headers Boost::unit_test_framework)

add_executable(search_test5 search_test5.cpp)
target_compile_definitions(search_test5 PRIVATE "BOOST_TEST_DYN_LINK=1")
target_link_libraries(search_test5 Boost::headers Boost::filesystem Boost::unit_test_framework)

add_executable(search_test6 search_test6.cpp)
target_compile_definitions(search_test6 PRIVATE "BOOST_TEST_DYN_LINK=1")
target_link_libraries(search_test6 Boost::headers Boost::unit_test_framework)

add_executable(search_test7 search_test7.cpp)
target_compile_definitions(search_test7 PRIVATE "BOOST_TEST_DYN_LINK=1")
target_link_libraries(search_test7 Boost::headers Boost::unit_test_framework)
