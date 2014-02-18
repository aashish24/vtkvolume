/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastAdditive.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers additive method.
// This test volume renders a synthetic dataset with unsigned char values,
// with the additive method.

#include "../Volume/vtkSinglePassSimpleVolumeMapper.h"

#include <vtkSphere.h>
#include <vtkSampleFunction.h>

#include <vtkTestUtilities.h>

#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVolumeProperty.h>
#include <vtkCamera.h>
#include <vtkRegressionTestImage.h>
#include <vtkImageShiftScale.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkRTAnalyticSource.h>
#include <vtkNew.h>
#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>

#include <vtkSmartPointer.h>
#include <vtksys/SystemTools.hxx>

#include <vtkImageReader.h>

int main(int argc, char *argv[])
{
  vtkNew<vtkSinglePassSimpleVolumeMapper> volumeMapper;
  volumeMapper->SetBlendModeToComposite();

  if (argc > 1)
    {
    std::string filename = argv[1];
    std::string ext = vtksys::SystemTools::GetFilenameLastExtension(filename);
    std::string filePathWOExt = vtksys::SystemTools::GetFilenameWithoutLastExtension(filename);
    if (ext == ".raw")
      {
      vtkNew<vtkImageReader> reader;
      reader->SetFileName(filename.c_str());
//      reader->SetDataByteOrderToBigEndian();
      reader->SetDataScalarTypeToUnsignedChar();
      reader->SetDataExtent(0, 255, 0, 255, 0, 255);
      reader->SetFileDimensionality(3);
//      reader->SetDataOrigin(0.0, 0.0, 0.0);
//      reader->SetNumberOfScalarComponents(1);
//      reader->SetFileLowerLeft(1);
      reader->Update();
      volumeMapper->SetInputConnection(reader->GetOutputPort());
      }
    }
  else
    {
    vtkNew<vtkRTAnalyticSource> source;
    source->Update();
    volumeMapper->SetInputConnection(source->GetOutputPort());
    }

  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> ren1;
  ren1->SetBackground(0.2, 0.2, 0.5);

  // Intentional odd and NPOT  width/height
  renWin->AddRenderer(ren1.GetPointer());
  ren1->Delete();
  renWin->SetSize(800, 800);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());
  renWin->Delete();

  // Make sure we have an OpenGL context.
  renWin->Render();

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());

  ren1->AddViewProp(volume.GetPointer());
  ren1->ResetCamera();

  renWin->Render();
  ren1->ResetCamera();

  iren->Start();
}

