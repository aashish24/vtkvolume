set(_srcs vtkSinglePassSimpleVolumeMapper.cxx GLSLShader.cxx)

add_library(vtkVolume ${_srcs})

target_link_libraries(vtkVolume GLEW)
