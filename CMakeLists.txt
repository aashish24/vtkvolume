cmake_minimum_required(VERSION 2.8.8)

# VTK is required
set(VTK_REQUIRED_COMPONENTS
  # Core components
  vtkCommonCore
  vtkFiltersCore
  vtkImagingCore
  vtkIOCore
  vtkViewsCore
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
  vtkTestingRendering
  vtkViewsContext2D
)
find_package(VTK NO_MODULE REQUIRED COMPONENTS ${VTK_REQUIRED_COMPONENTS})
include(${VTK_USE_FILE})

add_subdirectory(Volume)
add_subdirectory(OpenGL)
add_subdirectory(Testing)
add_subdirectory(Examples)
