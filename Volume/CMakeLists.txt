cmake_minimum_required(VERSION 2.8.8)
project(vtkVolume)

find_package(GLEW REQUIRED)

vtk_add_to_module_search_path(${vtkVolume_SOURCE_DIR} ${vtkVolume_BINARY_DIR})
set(vtkVolume_HEADERS vtkCubic.h GLSLShader.h vtkSinglePassSimpleVolumeMapper.h vtkTransferControlPoint.h
    vtkVector4d.h)
set(vtkVolume_SRCS vtkSinglePassVolumeMapper.cxx GLSLShader.cxx)

include(module.cmake)
vtk_module_library(${PROJECT_NAME} ${vtkVolume_SRCS})

target_link_libraries(${PROJECT_NAME} LINK_PRIVATE ${GLEW_LIBRARY})
