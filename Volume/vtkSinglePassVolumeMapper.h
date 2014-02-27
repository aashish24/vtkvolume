#ifndef __vtkSinglePassVolumeMapper_h
#define __vtkSinglePassVolumeMapper_h

#include <vtkVolumeMapper.h>

class vtkSinglePassVolumeMapper : public vtkVolumeMapper
{
  public:
    static vtkSinglePassVolumeMapper* New();

    vtkTypeMacro(vtkSinglePassVolumeMapper, vtkVolumeMapper);
    void PrintSelf( ostream& os, vtkIndent indent );

    virtual void Render(vtkRenderer *ren, vtkVolume *vol);

  protected:
    vtkSinglePassVolumeMapper();
    ~vtkSinglePassVolumeMapper();

    class vtkInternal;
    vtkInternal* Implementation;

private:
    vtkSinglePassVolumeMapper(const vtkSinglePassVolumeMapper&);  // Not implemented.
    void operator=(const vtkSinglePassVolumeMapper&);  // Not implemented.
};

#endif // __vtkSinglePassVolumeMapper_h
