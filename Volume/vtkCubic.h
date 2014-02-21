/// vtkCubic class that calculates the cubic spline from a set of control points/knots
/// and performs cubic interpolation.
///
/// Based on the natural cubic spline code from:
/// http://www.cse.unsw.edu.au/~lambert/splines/natcubic.html

#include <vector>

class vtkCubic
{
private:
  vtkVector4d a, b, c, d; // a + b*s + c*s^2 +d*s^3

public:
  vtkCubic()
    {
    }

  vtkCubic(vtkVector4d a, vtkVector4d b, vtkVector4d c, vtkVector4d d)
    {
    this->a = a;
    this->b = b;
    this->c = c;
    this->d = d;
    }

  // Evaluate the point using a cubic equation
  vtkVector4d GetPointOnSpline(float s)
    {
    return (((d * s) + c) * s + b) * s + a;
    }

  static std::vector<vtkCubic> CalculateCubicSpline(int n, std::vector<vtkTransferControlPoint> v)
    {
    std::vector<vtkVector4d> gamma;
    std::vector<vtkVector4d> delta;
    std::vector<vtkVector4d> D;

    gamma.resize(n + 1);
    delta.resize(n + 1);
    D.resize(n + 1);

    int i;
    /* We need to solve the equation
     * taken from: http://mathworld.wolfram.com/CubicSpline.html
       [2 1       ] [D[0]]   [3(v[1] - v[0])  ]
       |1 4 1     | |D[1]|   |3(v[2] - v[0])  |
       |  1 4 1   | | .  | = |      .         |
       |    ..... | | .  |   |      .         |
       |     1 4 1| | .  |   |3(v[n] - v[n-2])|
       [       1 2] [D[n]]   [3(v[n] - v[n-1])]

       by converting the matrix to upper triangular.
       The D[i] are the derivatives at the control points.
     */

    // This builds the coefficients of the left matrix
    gamma[0][0] = 1.0f / 2.0f;
    gamma[0][1] = 1.0f / 2.0f;
    gamma[0][2] = 1.0f / 2.0f;
    gamma[0][3] = 1.0f / 2.0f;

    for (i = 1; i < n; i++)
      {
      gamma[i] = vtkVector4d(1) / ((4 * vtkVector4d(1)) - gamma[i - 1]);
      }
    gamma[n] = vtkVector4d(1) / ((2 * vtkVector4d(1)) - gamma[n - 1]);

    delta[0] = 3 * (v[1].Color - v[0].Color) * gamma[0];
    for (i = 1; i < n; i++)
      {
      delta[i] = (3 * (v[i + 1].Color - v[i - 1].Color) - delta[i - 1]) * gamma[i];
      }
    delta[n] = (3 * (v[n].Color - v[n - 1].Color) - delta[n - 1]) * gamma[n];

    D[n] = delta[n];
    for (i = n - 1; i >= 0; i--)
      {
      D[i] = delta[i] - gamma[i] * D[i + 1];
      }

    // Now compute the coefficients of the cubics
    std::vector<vtkCubic> C;
    C.resize(n);
    for (i = 0; i < n; i++)
      {
      C[i] = vtkCubic(v[i].Color, D[i], 3 * (v[i + 1].Color - v[i].Color) - 2 * D[i] - D[i + 1],
             2 * (v[i].Color - v[i + 1].Color) + D[i] + D[i + 1]);
      }
    return C;
  }
};
