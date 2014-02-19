#ifndef __vtkTransferControlPoint_h
#define __vtkTransferControlPoint_h

#include "vtkVector4d.h"

// Class that implements control point for the transfer function
class vtkTransferControlPoint
{
public:
    vtkVector4d Color;
    int IsoValue;

    vtkTransferControlPoint(double r, double g, double b, int isovalue)
    {
      Color[0] = r;
      Color[1] = g;
      Color[2] = b;
      Color[3] = 1.0f;
      IsoValue = isovalue;
    }

    vtkTransferControlPoint(double alpha, int isovalue)
    {
      Color[0] = 0.0;
      Color[1] = 0.0;
      Color[2] = 0.0;
      Color[3] = alpha;
      IsoValue = isovalue;
    }
};

#endif // __vtkTransferControlPoint_h
