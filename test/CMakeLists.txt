# Unit tests for vmangos/core
# Requires -DBUILD_TESTS=ON

add_executable(vmangos_tests
    TestMain.cpp
    DatabaseTests.cpp
    BotTelemetryTests.cpp
    BotStanceTests.cpp
    BotPersonalityTests.cpp
)

target_include_directories(vmangos_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/shared
    ${CMAKE_SOURCE_DIR}/src/framework
    ${CMAKE_SOURCE_DIR}/dep/include
    ${MYSQL_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIRS}
)

target_link_libraries(vmangos_tests PRIVATE
    shared
    framework
    ${MYSQL_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${ZLIB_LIBRARIES}
)

if(WIN32)
  target_link_libraries(vmangos_tests PRIVATE ws2_32 mswsock winmm)
  # Same MySQL client libraries as the main binaries; ${MYSQL_LIBRARIES}
  # above is only set by the Unix find_package module.
  target_link_libraries(vmangos_tests PRIVATE
    optimized ${MYSQL_LIBRARY}
    debug ${MYSQL_DEBUG_LIBRARY}
  )
  target_compile_options(vmangos_tests PRIVATE /bigobj /EHsc)
else()
  find_package(Threads)
  target_link_libraries(vmangos_tests PRIVATE ${CMAKE_THREAD_LIBS_INIT})
  target_compile_options(vmangos_tests PRIVATE -Wall -Wextra -std=c++14)
endif()
