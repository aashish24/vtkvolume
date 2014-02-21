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
#include <vtkStructuredPointsReader.h>

#include <vtkSinglePassSimpleVolumeMapper.h>

int main(int argc, char *argv[])
{
  vtkNew<vtkSinglePassSimpleVolumeMapper> volumeMapper;
  volumeMapper->SetBlendModeToComposite();

  if (argc > 1)
    {
    std::string filename = argv[1];
    std::string ext = vtksys::SystemTools::GetFilenameLastExtension(filename);
    if (ext == ".vtk")
      {
      vtkNew<vtkStructuredPointsReader> reader;
      reader->SetFileName(argv[1]);
      reader->Update();
      volumeMapper->SetInputConnection(reader->GetOutputPort());
      }
    else
      {
      std::cerr << "File format " << ext << " is not supported " << std::endl;
      }
    }
  else
    {
    vtkNew<vtkRTAnalyticSource> source;
    source->Update();
    volumeMapper->SetInputConnection(source->GetOutputPort());
    }

  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0.2, 0.2, 0.5);

  // Intentional odd and NPOT  width/height
  renWin->AddRenderer(ren.GetPointer());
  renWin->SetSize(800, 800);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  // Make sure we have an OpenGL context.
  renWin->Render();

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());

  ren->AddViewProp(volume.GetPointer());
  ren->ResetCamera();

  renWin->Render();
  ren->ResetCamera();

  iren->Start();
}

