file(GLOB CONTROLLER_SOURCES
    "*.cpp"
    "*.h"
    "*.hpp"
)

add_library(controller STATIC ${CONTROLLER_SOURCES})
target_include_directories(controller PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(controller PUBLIC model view)
