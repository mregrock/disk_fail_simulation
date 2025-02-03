add_subdirectory(utils)
add_subdirectory(model)
add_subdirectory(view)
add_subdirectory(controller)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE 
    controller 
    model 
    view
    utils
    arctic_engine
)

IF (APPLE)
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${AUDIOTOOLBOX}
    ${COREAUDIO}
    ${COREFOUNDATION}
    ${COCOA}
    ${GAMECONTROLLER}
    ${OPENGL}
)
ELSE (APPLE)
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${OPENGL_gl_LIBRARY}
    ${X11_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${ALSA_LIBRARY}
)
ENDIF (APPLE)
