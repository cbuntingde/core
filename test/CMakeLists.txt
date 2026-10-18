# Unit tests for vmangos/core
# Requires -DBUILD_TESTS=ON

add_executable(vmangos_tests
    TestMain.cpp
    DatabaseTests.cpp
    QueryResultStmtTests.cpp
    BotTelemetryTests.cpp
    BotStanceTests.cpp
    BotPersonalityTests.cpp
    BotHealerTests.cpp
    BotObjectiveTests.cpp
    BotDungeonTests.cpp
    BotPerfTests.cpp
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
    ${MYSQL_LIBRARY}
    ${OPENSSL_LIBRARIES}
    ${ZLIB_LIBRARIES}
)

if(WIN32)
  target_link_libraries(vmangos_tests PRIVATE ws2_32 mswsock winmm)
  # The release client library is linked above for every platform; only the
  # debug variant is Windows-specific. FindMySQL.cmake defines MYSQL_LIBRARY
  # and never MYSQL_LIBRARIES, so naming the plural left vmangos_tests without
  # the MySQL client on Unix and failed to link mysql_thread_init/end.
  target_link_libraries(vmangos_tests PRIVATE
    debug ${MYSQL_DEBUG_LIBRARY}
  )
  target_compile_options(vmangos_tests PRIVATE /bigobj /EHsc)
else()
  find_package(Threads)
  target_link_libraries(vmangos_tests PRIVATE ${CMAKE_THREAD_LIBS_INIT})
  # No explicit -std flag: inherit CMAKE_CXX_STANDARD so the tests build at the
  # same language level as the code they cover.
  target_compile_options(vmangos_tests PRIVATE -Wall -Wextra)
endif()
