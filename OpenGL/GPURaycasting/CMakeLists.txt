cmake_minimum_required(VERSION 2.8.8)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(GLEW REQUIRED)

add_executable(app main.cpp GLSLShader.cpp RenderableObject.cpp Grid.cpp)
target_link_libraries(app ${GLEW_LIBRARY} ${OPENGL_gl_LIBRARY} ${GLUT_LIBRARY})
