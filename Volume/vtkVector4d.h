#ifndef __vtkVector4d_h
#define __vtkVector4d_h

#include <vtkSetGet.h>
#include <vtkVector.h>

class vtkVector4d : public vtkVector<double, 4>
{
public:
  vtkVector4d() {}
  vtkVector4d(double x, double y, double z, double w) : vtkVector<double, 4>()
    {
    this->Data[0] = x;
    this->Data[1] = y;
    this->Data[2] = z;
    this->Data[3] = w;
    }
  explicit vtkVector4d(double scalar) : vtkVector<double, 4>(scalar)
    {
    this->Data[0] = scalar;
    this->Data[1] = scalar;
    this->Data[2] = scalar;
    this->Data[3] = scalar;
    }
  explicit vtkVector4d(const double *init) : vtkVector<double, 4>(init) {}
  vtkVectorDerivedMacro(vtkVector4d, double, 4)
//  vtkVector4Cross(vtkVector4d, double)
};

vtkVector4d operator*(const vtkVector4d& vec4d, float scalar)
{
    vtkVector4d out;
    out[0] = vec4d[0] * scalar;
    out[1] = vec4d[1] * scalar;
    out[2] = vec4d[2] * scalar;
    out[3] = vec4d[3] * scalar;

    return out;
}

vtkVector4d operator*(int scalar, const vtkVector4d& vec4d)
{
    vtkVector4d out;
    out[0] = vec4d[0] * scalar;
    out[1] = vec4d[1] * scalar;
    out[2] = vec4d[2] * scalar;
    out[3] = vec4d[3] * scalar;

    return out;
}

vtkVector4d operator*(const vtkVector4d& vec4d1, const vtkVector4d& vec4d2)
{
    vtkVector4d out;
    out[0] = vec4d1[0] * vec4d2[0];
    out[1] = vec4d1[1] * vec4d2[1];
    out[2] = vec4d1[2] * vec4d2[2];
    out[3] = vec4d1[3] * vec4d2[3];

    return out;
}

vtkVector4d operator/(const vtkVector4d& vec4d1, const vtkVector4d& vec4d2)
{
    vtkVector4d out;
    out[0] = vec4d1[0] / vec4d2[0];
    out[1] = vec4d1[1] / vec4d2[1];
    out[2] = vec4d1[2] / vec4d2[2];
    out[3] = vec4d1[3] / vec4d2[3];

    return out;
}

vtkVector4d operator+(const vtkVector4d& vec4d1, const vtkVector4d& vec4d2)
{
    vtkVector4d out;
    out[0] = vec4d1[0] + vec4d2[0];
    out[1] = vec4d1[1] + vec4d2[1];
    out[2] = vec4d1[2] + vec4d2[2];
    out[3] = vec4d1[3] + vec4d2[3];

    return out;
}

vtkVector4d operator-(const vtkVector4d& vec4d1, const vtkVector4d& vec4d2)
{
    vtkVector4d out;
    out[0] = vec4d1[0] - vec4d2[0];
    out[1] = vec4d1[1] - vec4d2[1];
    out[2] = vec4d1[2] - vec4d2[2];
    out[3] = vec4d1[3] - vec4d2[3];

    return out;
}

#endif // __vtkVector4d_h
