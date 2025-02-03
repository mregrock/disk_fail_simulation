file(GLOB MODEL_SOURCES
    "*.cpp"
    "*.h"
    "*.hpp"
)

add_library(model STATIC ${MODEL_SOURCES})
target_include_directories(model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
