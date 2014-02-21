
set(_headers vtkCubic.h GLSLShader.h vtkSinglePassSimpleVolumeMapper.h vtkTransferControlPoint.h
    vtkVector4d.h)
set(_srcs vtkSinglePassSimpleVolumeMapper.cxx GLSLShader.cxx)

add_library(vtkVolume ${_headers} ${_srcs})
target_link_libraries(vtkVolume GLEW)
