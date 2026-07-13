
add_executable(microbox-tests
  test_2048.cpp
  test_geolocation_api.cpp
)

# We link tests against the 'core library' that contains all of our microbox code.
target_link_libraries(microbox-tests PRIVATE
  microbox-core
  Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(microbox-tests)
