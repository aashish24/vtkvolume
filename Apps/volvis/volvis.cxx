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
#include <vtkCommand.h>
#include <vtkFixedPointVolumeRayCastMapper.h>
#include <vtkGPUVolumeRayCastMapper.h>
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

#include <vtkSinglePassVolumeMapper.h>

#include <vtkTimerLog.h>

#include <cstdlib>

/// Testing
class vtkTimerCallback : public vtkCommand
{
  public:
    static vtkTimerCallback *New()
    {
      vtkTimerCallback *cb = new vtkTimerCallback;
      cb->TimerCount = 0;
      return cb;
    }

    void Set(vtkRenderWindow* rw, vtkRenderer* ren)
    {
      this->RenderWindow = rw;
      this->Renderer = ren;
    }

    virtual void Execute(vtkObject *vtkNotUsed(caller), unsigned long eventId,
                         void *vtkNotUsed(callData))
    {
      static int counter = 0;
      static vtkNew<vtkTimerLog> timer;
      static double totalTime = 0;
      static double totalFrames = 0;

      if (counter ==  100)
        {
        timer->StartTimer();
        }
      if (vtkCommand::TimerEvent == eventId)
        {
        ++this->TimerCount;
        }
      this->Renderer->GetActiveCamera()->Azimuth(1);
      this->RenderWindow->Render();

      if (counter >  100)
        {
        ++totalFrames;
        }

      ++counter;
      if (counter == 200)
        {
        timer->StopTimer();
        totalTime += timer->GetElapsedTime();

        std::cerr << "totalFrames " << totalFrames << std::endl;
        std::cerr << "totalTime " << totalTime << std::endl;
        std::cerr << "FPS " << totalFrames / totalTime << std::endl;

        totalFrames = 0;
        timer->ResetLog();

        exit(0);
        }
    }

  private:
    int TimerCount;
    vtkRenderer* Renderer;
    vtkRenderWindow* RenderWindow;

};

int main(int argc, char *argv[])
{
  bool testing = false;
  double scalarRange[2];

  vtkSmartPointer<vtkVolumeMapper> volumeMapper;

  if (argc > 1)
    {
    for (int i = 1; i < argc; ++i)
      {
      std::string arg = argv[i];

      if (arg == "-fp" || arg == "-gp" || arg == "-sp")
        {
        if (arg == "-fp")
          {
          volumeMapper = vtkSmartPointer<vtkFixedPointVolumeRayCastMapper>(
                           vtkFixedPointVolumeRayCastMapper::New());
          }
        else if (arg == "-gp")
          {
          volumeMapper = vtkSmartPointer<vtkGPUVolumeRayCastMapper>(
                           vtkGPUVolumeRayCastMapper::New());
          }
        else
          {
          volumeMapper = vtkSmartPointer<vtkSinglePassVolumeMapper>(
                           vtkSinglePassVolumeMapper::New());
          }
        }
      else if (arg == "-t")
        {
        testing = true;
        }
      else
        {
        // Deault is single pass volume mapper
        if (!volumeMapper)
          {
          volumeMapper = vtkSmartPointer<vtkSinglePassVolumeMapper>(
                           vtkSinglePassVolumeMapper::New());
          }

        std::string ext = vtksys::SystemTools::GetFilenameLastExtension(arg);
        if (ext == ".vtk")
          {
          vtkNew<vtkStructuredPointsReader> reader;
          reader->SetFileName(arg.c_str());
          reader->Update();
          volumeMapper->SetInputConnection(reader->GetOutputPort());
          }
        }
      }
    }
  else
    {
    vtkNew<vtkRTAnalyticSource> source;
    source->Update();
    volumeMapper->SetInputConnection(source->GetOutputPort());
    }

  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetBlendModeToComposite();

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

  vtkPiecewiseFunction* scalarOpacity = vtkPiecewiseFunction::New();
  // Keeping the same opacity table for different mappers
  scalarOpacity->AddPoint(scalarRange[0], 0.0);
  scalarOpacity->AddPoint(scalarRange[1], 1.0);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  vtkColorTransferFunction* colorTransferFunction = volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(scalarRange[1], 1.0, 1.0, 1.0);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty.GetPointer());

  ren->AddViewProp(volume.GetPointer());
  ren->ResetCamera();

  renWin->Render();
  ren->ResetCamera();

  /// Testing code
  if (testing)
    {
    vtkNew<vtkTimerCallback> cb;
    cb->Set(renWin.GetPointer(), ren.GetPointer());
    iren->AddObserver(vtkCommand::TimerEvent, cb.GetPointer());
    iren->CreateRepeatingTimer(10);
    }

  iren->Start();
}

