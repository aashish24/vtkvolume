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

#include "vtkSphere.h"
#include "vtkSampleFunction.h"

#include "vtkTestUtilities.h"
#include "vtkColorTransferFunction.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkVolumeProperty.h"
#include "vtkCamera.h"
#include "vtkRegressionTestImage.h"
#include "vtkImageShiftScale.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkDataArray.h"
#include "vtkRTAnalyticSource.h"
#include "vtkNew.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"

int main(int argc, char *argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  vtkRTAnalyticSource* source=vtkRTAnalyticSource::New();
  source->SetCenter(0.0, 0.0, 0.0);
  source->SetWholeExtent(-2, 2, -2, 2, -2, 2);
  source->Update();

  vtkRenderWindow* renWin=vtkRenderWindow::New();
  vtkRenderer *ren1 = vtkRenderer::New();
  ren1->SetBackground(0.2, 0.2, 0.5);

  // intentional odd and NPOT  width/height
  renWin->AddRenderer(ren1);
  ren1->Delete();
  renWin->SetSize(300, 300);

  vtkRenderWindowInteractor *iren=vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  renWin->Delete();

  // make sure we have an OpenGL context.
  renWin->Render();

  vtkSinglePassSimpleVolumeMapper* volumeMapper;
  vtkVolumeProperty* volumeProperty;
  vtkVolume* volume;

  volumeMapper = vtkSinglePassSimpleVolumeMapper::New();

  // TODO Fix this
  volumeMapper->SetBlendModeToComposite(); // composite first
  volumeMapper->SetInputConnection(source->GetOutputPort());

  volumeProperty=vtkVolumeProperty::New();
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);

  volume = vtkVolume::New();
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);
  ren1->AddViewProp(volume);

  ren1->ResetCamera();
  renWin->Render();
  ren1->ResetCamera();
  iren->Start();

  volumeMapper->Delete();
  volumeProperty->Delete();
  volume->Delete();
  iren->Delete();
}

