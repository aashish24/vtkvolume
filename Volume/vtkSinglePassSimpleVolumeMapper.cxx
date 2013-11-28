#include "vtkSinglePassSimpleVolumeMapper.h"

#include <vtkObjectFactory.h>

vtkStandardNewMacro(vtkSinglePassSimpleVolumeMapper);

vtkSinglePassSimpleVolumeMapper::vtkSinglePassSimpleVolumeMapper() : vtkVolumeMapper()
{
}


vtkSinglePassSimpleVolumeMapper::~vtkSinglePassSimpleVolumeMapper()
{
}


void vtkSinglePassSimpleVolumeMapper::PrintSelf(ostream &os, vtkIndent indent)
{
  // TODO Implement this method
}


void vtkSinglePassSimpleVolumeMapper::Render(vtkRenderer *ren, vtkVolume *vol)
{
}






