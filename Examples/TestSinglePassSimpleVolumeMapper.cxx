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

int main(int argc, char *argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  // Create a spherical implicit function.
  vtkSphere* shape = vtkSphere::New();
  shape->SetRadius(0.1);
  shape->SetCenter(0.0,0.0,0.0);

  vtkSampleFunction* source=vtkSampleFunction::New();
  source->SetImplicitFunction(shape);
  shape->Delete();
  source->SetOutputScalarTypeToDouble();
  source->SetSampleDimensions(127,127,127); // intentional NPOT dimensions.
  source->SetModelBounds(-1.0,1.0,-1.0,1.0,-1.0,1.0);
  source->SetCapping(false);
  source->SetComputeNormals(false);
  source->SetScalarArrayName("values");
  source->Update();

  vtkDataArray *a=source->GetOutput()->GetPointData()->GetScalars("values");
  double range[2];
  a->GetRange(range);

  vtkImageShiftScale* t = vtkImageShiftScale::New();
  t->SetInputConnection(source->GetOutputPort());
  source->Delete();
  t->SetShift(-range[0]);
  double magnitude=range[1]-range[0];
  if(magnitude==0.0)
    {
    magnitude=1.0;
    }
  t->SetScale(255.0/magnitude);
  t->SetOutputScalarTypeToUnsignedChar();
  t->Update();

  vtkRenderWindow *renWin=vtkRenderWindow::New();
  vtkRenderer *ren1=vtkRenderer::New();
  ren1->SetBackground(0.1,0.4,0.2);

  renWin->AddRenderer(ren1);
  ren1->Delete();
  renWin->SetSize(301,300); // intentional odd and NPOT  width/height

  vtkRenderWindowInteractor *iren=vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  renWin->Delete();

  renWin->Render(); // make sure we have an OpenGL context.

  vtkSinglePassSimpleVolumeMapper* volumeMapper;
  vtkVolumeProperty* volumeProperty;
  vtkVolume* volume;

  volumeMapper = vtkSinglePassSimpleVolumeMapper::New();

  // TODO Fix this
  //volumeMapper->SetBlendModeToComposite(); // composite first

  volumeMapper->SetInputConnection(t->GetOutputPort());

  volumeProperty=vtkVolumeProperty::New();
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);

  // TODO Fix this
//  vtkPiecewiseFunction* additiveOpacity = vtkPiecewiseFunction::New();
//  additiveOpacity->AddPoint(0.0,0.0);
//  additiveOpacity->AddPoint(200.0,0.5);
//  additiveOpacity->AddPoint(200.1,1.0);
//  additiveOpacity->AddPoint(255.0,1.0);

//  vtkPiecewiseFunction *compositeOpacity = vtkPiecewiseFunction::New();
//  compositeOpacity->AddPoint(0.0,0.0);
//  compositeOpacity->AddPoint(80.0,1.0);
//  compositeOpacity->AddPoint(80.1,0.0);
//  compositeOpacity->AddPoint(255.0,0.0);
//  volumeProperty->SetScalarOpacity(compositeOpacity); // composite first.

//  vtkColorTransferFunction *color=vtkColorTransferFunction::New();
//  color->AddRGBPoint(0.0  ,0.0,0.0,1.0);
//  color->AddRGBPoint(40.0  ,1.0,0.0,0.0);
//  color->AddRGBPoint(255.0,1.0,1.0,1.0);
//  volumeProperty->SetColor(color);
//  color->Delete();

  volume = vtkVolume::New();
  volume->SetMapper(volumeMapper);
//  volume->SetProperty(volumeProperty);
  ren1->AddViewProp(volume);

  // TODO Fix this
//  int valid = volumeMapper->IsRenderSupported(renWin, volumeProperty);

  int valid = 1;
  int retVal;
  if(valid)
    {
    ren1->ResetCamera();

    // TODO Fix this
//    // Render composite.
//    renWin->Render();

//    // Switch to Additive
//    volumeMapper->SetBlendModeToAdditive();
//    volumeProperty->SetScalarOpacity(additiveOpacity);
    renWin->Render();
    iren->Start();
    }

  volumeMapper->Delete();
  volumeProperty->Delete();
  volume->Delete();
  iren->Delete();
  t->Delete();

//  additiveOpacity->Delete();
//  compositeOpacity->Delete();

  return !((retVal == vtkTesting::PASSED) || (retVal == vtkTesting::DO_INTERACTOR));
}

