project(volvis)

set(VOLVIS_SRCS volvis.cxx)

add_executable(volvis  "${VOLVIS_SRCS}")

include_directories(SYSTEM
  ${VTK_INCLUDE_DIRS}
  ${vtkNextGenVolumeRendering_SOURCE_DIR}/Volume
)

target_link_libraries(volvis
  vtkRenderingVolumeOpenGL vtkRenderingOpenGL vtkFiltersGeneral
  vtkImagingHybrid vtkRenderingFreeTypeOpenGL vtkInteractionStyle
  vtkTestingRendering vtkFiltersSources vtksys vtkIOLegacy
  vtkVolume
)

