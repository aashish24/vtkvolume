set(example_srcs
  GPURayCastAdditiveOld.cxx
  TestSinglePassSimpleVolumeMapper.cxx
)

foreach(example_src ${example_srcs})
  get_filename_component(example_name "${example_src}" NAME_WE)
  add_executable("${example_name}"  "${example_src}")
  target_link_libraries("${example_name}"
    vtkRenderingVolumeOpenGL vtkRenderingOpenGL vtkFiltersGeneral
    vtkImagingHybrid vtkRenderingFreeTypeOpenGL vtkInteractionStyle
    vtkTestingRendering vtkFiltersSources
    vtkVolume
  )
endforeach()
