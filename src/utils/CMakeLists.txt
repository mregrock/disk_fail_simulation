file(GLOB UTILS_SOURCES
    "*.cpp"
    "*.h"
    "*.hpp"
)

add_library(utils STATIC ${UTILS_SOURCES})
target_include_directories(utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}) 