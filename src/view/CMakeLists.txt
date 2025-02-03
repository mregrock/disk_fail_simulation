file(GLOB VIEW_SOURCES
    "*.cpp"
    "*.h"
    "*.hpp"
)

add_library(view STATIC ${VIEW_SOURCES})
target_include_directories(view PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(view PUBLIC model)
