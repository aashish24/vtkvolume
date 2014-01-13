#ifndef __vtkSinglePassSimpleVolumeMapper_h
#define __vtkSinglePassSimpleVolumeMapper_h

#include <vtkVolumeMapper.h>

class vtkSinglePassSimpleVolumeMapper : public vtkVolumeMapper
{
  public:
    static vtkSinglePassSimpleVolumeMapper* New();

    vtkTypeMacro(vtkSinglePassSimpleVolumeMapper, vtkVolumeMapper);
    void PrintSelf( ostream& os, vtkIndent indent );

    virtual void Render(vtkRenderer *ren, vtkVolume *vol);

  protected:
    vtkSinglePassSimpleVolumeMapper();
    ~vtkSinglePassSimpleVolumeMapper();

    class vtkInternal;
    vtkInternal* Implementation;

private:
    vtkSinglePassSimpleVolumeMapper(const vtkSinglePassSimpleVolumeMapper&);  // Not implemented.
    void operator=(const vtkSinglePassSimpleVolumeMapper&);  // Not implemented.
};

#endif // __vtkSinglePassSimpleVolumeMapper_h
