cmake_minimum_required(VERSION 3.9)
project(qt-rappor-client)

find_package(Qt5 COMPONENTS Core REQUIRED)

add_library(qt-rappor
    encoder.cc
    qt_hash_impl.cc
    std_rand_impl.cc
    )
target_link_libraries(qt-rappor Qt5::Core)

add_executable(encoder_demo encoder_demo.cc)
target_link_libraries(encoder_demo qt-rappor)

add_executable(rappor_sim rappor_sim.cc)
target_link_libraries(rappor_sim qt-rappor)

find_package(GTest)
if (GTEST_FOUND)
    include(CTest)

    # This never passed, as far back in the git history as I could go
    add_executable(encoder_unittest encoder_unittest.cc mock_rand_impl.cc)

    # This passes
    add_executable(openssl_hash_impl_unittest openssl_hash_impl_unittest.cc)

    target_link_libraries(openssl_hash_impl_unittest qt-rappor GTest::GTest)
    add_test(NAME openssl_hash_impl_unittest COMMAND openssl_hash_impl_unittest)
    target_link_libraries(encoder_unittest qt-rappor GTest::GTest)
    add_test(NAME encoder_unittest COMMAND encoder_unittest)
else()
    message(STATUS "Skipping tests")
endif()

install(TARGETS qt-rappor DESTINATION lib)

install(FILES encoder.h DESTINATION include/qt-rappor)
install(FILES rappor_deps.h DESTINATION include/qt-rappor)
install(FILES std_rand_impl.h DESTINATION include/qt-rappor)
install(FILES qt_hash_impl.h DESTINATION include/qt-rappor)