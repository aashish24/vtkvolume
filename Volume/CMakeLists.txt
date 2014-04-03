cmake_minimum_required(VERSION 2.8.8)
project(vtkVolume)

find_package(GLEW REQUIRED)

vtk_add_to_module_search_path(${vtkVolume_SOURCE_DIR} ${vtkVolume_BINARY_DIR})
set(vtkVolume_HEADERS vtkCubic.h GLSLShader.h vtkSinglePassVolumeMapper.h vtkTransferControlPoint.h
    vtkVector4d.h vtkOpenGLVolumeRGBTable.h vtkOpenGLOpacityTable.h)
set(vtkVolume_SRCS vtkSinglePassVolumeMapper.cxx GLSLShader.cxx)

include(module.cmake)
vtk_module_library(${PROJECT_NAME} ${vtkVolume_HEADERS} ${vtkVolume_SRCS})

target_link_libraries(${PROJECT_NAME} LINK_PRIVATE ${GLEW_LIBRARY})

add_custom_command(OUTPUT "${vtkVolume_BINARY_DIR}/shaders"
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   "${vtkVolume_SOURCE_DIR}/shaders" "${CMAKE_BINARY_DIR}/shaders")
add_custom_target(copy_shaders ALL DEPENDS "${vtkVolume_BINARY_DIR}/shaders")

add_dependencies(vtkVolume copy_shaders)

