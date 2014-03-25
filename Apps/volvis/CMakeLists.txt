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

add_custom_command(OUTPUT "${volvis_BINARY_DIR}/shaders"
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   "${volvis_SOURCE_DIR}/shaders" "${volvis_BINARY_DIR}/shaders")
add_custom_target(copy_shaders ALL DEPENDS "${volvis_BINARY_DIR}/shaders")

add_dependencies(volvis copy_shaders)
