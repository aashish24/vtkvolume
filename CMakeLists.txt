cmake_minimum_required(VERSION 2.8.8)

project(vtkNextGenVolumeRendering)

# VTK is required
set(VTK_REQUIRED_COMPONENTS
  # Core components
  vtkCommonCore
  vtkFiltersCore
  vtkImagingCore
  vtkIOCore
  vtkViewsCore
  vtkVolume
  # Additional features
  vtkPython
  vtkWrappingPythonCore
  vtkCommonColor
  vtkCommonDataModel
  vtkCommonExecutionModel
  vtkCommonMath
  vtkCommonSystem
  vtkFiltersExtraction
  vtkFiltersGeneral
  vtkFiltersHybrid
  vtkFiltersModeling
  vtkFiltersSources
  vtkFiltersStatistics
  vtkFiltersTexture
  vtkImagingHybrid
  vtkImagingStencil
  vtkInfovisLayout
  vtkInteractionStyle
  vtkInteractionWidgets
  vtkIOImage
  vtkIOLegacy
  vtkIOXML
  vtkRenderingAnnotation
  vtkRenderingFreeTypeOpenGL
  vtkRenderingLabel
  vtkRenderingOpenGL
  vtkRenderingVolumeOpenGL
  vtksys
  vtkViewsContext2D
)
find_package(VTK NO_MODULE REQUIRED COMPONENTS ${VTK_REQUIRED_COMPONENTS})
include(${VTK_USE_FILE})

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${VTK_CMAKE_DIR})
include(vtkExternalModuleMacros)

add_subdirectory(OpenGL)
add_subdirectory(Testing)
add_subdirectory(Apps)
