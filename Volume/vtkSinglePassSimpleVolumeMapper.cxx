#include "vtkSinglePassSimpleVolumeMapper.h"

#include "GLSLShader.h"

#include <vtkObjectFactory.h>
#include <vtkCamera.h>
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
#include <vtkPointData.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkUnsignedCharArray.h>
#include <vtkSmartPointer.h>
#include <vtkPoints.h>
#include <vtkFloatArray.h>
#include <vtkVector.h>

#include <GL/glew.h>
#include <vtkgl.h>

#include <cassert>

vtkStandardNewMacro(vtkSinglePassSimpleVolumeMapper);

// Remove this afterwards
#define GL_CHECK_ERRORS \
  {\
  std::cerr << "Checking for error " << glGetError() << std::endl; \
  assert(glGetError()== GL_NO_ERROR); \
  }

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

// Class that implements control point for the transfer function
class TransferControlPoint
{
public:
    vtkVector4d Color;
    int IsoValue;

    TransferControlPoint(double r, double g, double b, int isovalue)
    {
      Color[0] = r;
      Color[1] = g;
      Color[2] = b;
      Color[3] = 1.0f;
      IsoValue = isovalue;
    }

    TransferControlPoint(double alpha, int isovalue)
    {
      Color[0] = 0.0;
      Color[1] = 0.0;
      Color[2] = 0.0;
      Color[3] = alpha;
      IsoValue = isovalue;
    }
};

/// Cubic class that calculates the cubic spline from a set of control points/knots
/// and performs cubic interpolation.
///
/// Based on the natural cubic spline code from:
/// http://www.cse.unsw.edu.au/~lambert/splines/natcubic.html
class Cubic
{
private:
  vtkVector4d a, b, c, d; // a + b*s + c*s^2 +d*s^3

public:
  Cubic()
    {
    }

  Cubic(vtkVector4d a, vtkVector4d b, vtkVector4d c, vtkVector4d d)
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

  static std::vector<Cubic> CalculateCubicSpline(int n, std::vector<TransferControlPoint> v)
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
    std::vector<Cubic> C;
    C.resize(n);
    for (i = 0; i < n; i++)
      {
      C[i] = Cubic(v[i].Color, D[i], 3 * (v[i + 1].Color - v[i].Color) - 2 * D[i] - D[i + 1],
             2 * (v[i].Color - v[i + 1].Color) + D[i] + D[i + 1]);
      }
    return C;
  }
};

// Class that hides implementation details
class vtkSinglePassSimpleVolumeMapper::vtkInternal
{
public:
  vtkInternal(vtkSinglePassSimpleVolumeMapper* parent) :
    Parent(parent),
    VolmeLoaded(false), Initialized(false), ValidTransferFunction(false)
    {
    this->ColorKnots = std::vector<TransferControlPoint>();
    this->ColorKnots.push_back(TransferControlPoint(0.0, 0.0, 0.0, 0));
    this->ColorKnots.push_back(TransferControlPoint(1.0, 1.0, 1.0, 50));
    this->ColorKnots.push_back(TransferControlPoint(1.0, 0.8, 0.8, 200));
    this->ColorKnots.push_back(TransferControlPoint(1.0, 0.5, 0.5, 256));
    this->ColorKnots.push_back(TransferControlPoint(1.0, 0.2, 0.2, 512));

    this->AlphaKnots = std::vector<TransferControlPoint>();
    this->AlphaKnots.push_back(TransferControlPoint(0.0f, 0));
    this->AlphaKnots.push_back(TransferControlPoint(0.0f, 50));
    this->AlphaKnots.push_back(TransferControlPoint(0.0051f, 80));
    this->AlphaKnots.push_back(TransferControlPoint(0.01f, 500));
    this->AlphaKnots.push_back(TransferControlPoint(0.02f, 512));
    }

  ~vtkInternal()
    {
    }

  unsigned int GetVolumeTexture()
    {
    return this->VolumeTexture;
    }

  unsigned int GetTransferFunctionSampler()
    {
    return this->TransferFuncSampler;
    }

  bool LoadVolume(vtkImageData* imageData);
  bool IsVolmeLoaded();
  bool IsInitialized();
  bool HasValidTransferFunction();
  void ComputeTransferFunction();

  vtkSinglePassSimpleVolumeMapper* Parent;

  bool VolmeLoaded;
  bool Initialized;
  bool ValidTransferFunction;

  GLuint cubeVBOID;
  GLuint cubeVAOID;
  GLuint cubeIndicesID;

  GLSLShader shader;

  GLuint VolumeTexture;
  GLuint TransferFuncSampler;

  int CellFlag;
  int TextureSize[3];
  int TextureExtents[6];

  std::vector<TransferControlPoint> ColorKnots;
  std::vector<TransferControlPoint> AlphaKnots;
};

// Helper method for computing the transfer function
void vtkSinglePassSimpleVolumeMapper::vtkInternal::ComputeTransferFunction()
{
  const int width = 512;
  // Initialize the cubic spline for the transfer function
  float transferFunc[width*4];

  // Initialize to 0
  for (int i = 0; i < width * 4; i++)
    {
    transferFunc[i] = 0.0;
    }

  // Fit a cubic spline for the transfer function
  std::vector<Cubic> colorCubic = Cubic::CalculateCubicSpline(ColorKnots.size() - 1, ColorKnots);
  std::vector<Cubic> alphaCubic = Cubic::CalculateCubicSpline(AlphaKnots.size() - 1, AlphaKnots);

  int numTF = 0;
  for (int i = 0; i < ColorKnots.size() - 1; i++)
    {
    // Compute steps
    int steps = ColorKnots[i + 1].IsoValue - ColorKnots[i].IsoValue;

    // Now compute the color
    for (int j = 0; j < steps; j++)
      {
      float k = (float)j / (float)(steps - 1);

      vtkVector4d color = colorCubic[i].GetPointOnSpline(k);
      for (int k = 0; k < 4; ++k)
        {
        transferFunc[numTF++] = color[k];
        }
      }
    }

  numTF = 0;
  for (int i = 0; i < AlphaKnots.size() - 1; i++)
    {
    int steps = AlphaKnots[i + 1].IsoValue - AlphaKnots[i].IsoValue;
    for (int j = 0; j < steps; j++)
      {
      float k = (float)j / (float)(steps - 1);

      vtkVector4d color = alphaCubic[i].GetPointOnSpline(k);
      transferFunc[4 * (++numTF) - 1] = color[3];
      std::cerr << color[3] << std::endl;
      }
    }

  // Generate OpenGL texture
  glEnable(GL_TEXTURE_1D);
  glGenTextures(1, &this->TransferFuncSampler);
  glBindTexture(GL_TEXTURE_1D, this->TransferFuncSampler);

  // Set the texture parameters
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_R, GL_CLAMP);

  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, width, 0, GL_RGBA, GL_FLOAT, transferFunc);

  this->ValidTransferFunction = true;
}


bool vtkSinglePassSimpleVolumeMapper::vtkInternal::LoadVolume(vtkImageData* imageData)
{
  // Generate OpenGL texture
  glEnable(GL_TEXTURE_3D);
  glGenTextures(1, &this->VolumeTexture);
  glBindTexture(GL_TEXTURE_3D, this->VolumeTexture);

  // Set the texture parameters
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

  // Set the mipmap levels (base and max)
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 4);

  GL_CHECK_ERRORS

  // Allocate data with internal format and foramt as (GL_RED)
  GLint internalFormat=0;
  GLenum format=0;
  GLenum type=0;

  double shift=0.0;
  double scale=1.0;
  int needTypeConversion=0;

 // @aashish: tableRange is for transform function in existing VTK code.
 double tableRange[2];

  vtkDataArray* scalars = this->Parent->GetScalars(imageData,
                          this->Parent->ScalarMode,
                          this->Parent->ArrayAccessMode,
                          this->Parent->ArrayId,
                          this->Parent->ArrayName,
                          this->CellFlag);

  int scalarType=scalars->GetDataType();
  if(scalars->GetNumberOfComponents()==4)
    {
    // this is RGBA, unsigned char only
    internalFormat=GL_RGBA16;
    format=GL_RGBA;
    type=GL_UNSIGNED_BYTE;
    }
  else
    {
    switch(scalarType)
      {
      case VTK_FLOAT:
//          if(this->Supports_GL_ARB_texture_float)
//            {
//            internalFormat=vtkgl::INTENSITY16F_ARB;
//            }
//          else
//            {
            internalFormat=GL_INTENSITY16;
//            }
        format=GL_RED;
        type=GL_FLOAT;
        shift=-tableRange[0];
        scale=1/(tableRange[1]-tableRange[0]);
        break;
      case VTK_UNSIGNED_CHAR:
        internalFormat=GL_INTENSITY8;
        format=GL_RED;
        type=GL_UNSIGNED_BYTE;
        shift=-tableRange[0]/VTK_UNSIGNED_CHAR_MAX;
        scale=
          VTK_UNSIGNED_CHAR_MAX/(tableRange[1]-tableRange[0]);
        break;
      case VTK_SIGNED_CHAR:
        internalFormat=GL_INTENSITY8;
        format=GL_RED;
        type=GL_BYTE;
        shift=-(2*tableRange[0]+1)/VTK_UNSIGNED_CHAR_MAX;
        scale=VTK_SIGNED_CHAR_MAX/(tableRange[1]-tableRange[0]);
        break;
      case VTK_CHAR:
        // not supported
        assert("check: impossible case" && 0);
        break;
      case VTK_BIT:
        // not supported
        assert("check: impossible case" && 0);
        break;
      case VTK_ID_TYPE:
        // not supported
        assert("check: impossible case" && 0);
        break;
      case VTK_INT:
        internalFormat=GL_INTENSITY16;
        format=GL_RED;
        type=GL_INT;
        shift=-(2*tableRange[0]+1)/VTK_UNSIGNED_INT_MAX;
        scale=VTK_INT_MAX/(tableRange[1]-tableRange[0]);
        break;
      case VTK_DOUBLE:
      case VTK___INT64:
      case VTK_LONG:
      case VTK_LONG_LONG:
      case VTK_UNSIGNED___INT64:
      case VTK_UNSIGNED_LONG:
      case VTK_UNSIGNED_LONG_LONG:
//          needTypeConversion=1; // to float
//          if(this->Supports_GL_ARB_texture_float)
//            {
//            internalFormat=vtkgl::INTENSITY16F_ARB;
//            }
//          else
//            {
//            internalFormat=GL_INTENSITY16;
//            }
//          format=GL_RED;
//          type=GL_FLOAT;
//          shift=-tableRange[0];
//          scale=1/(tableRange[1]-tableRange[0]);
//          sliceArray=vtkFloatArray::New();
        break;
      case VTK_SHORT:
        internalFormat=GL_INTENSITY16;
        format=GL_RED;
        type=GL_SHORT;
        shift=-(2*tableRange[0]+1)/VTK_UNSIGNED_SHORT_MAX;
        scale=VTK_SHORT_MAX/(tableRange[1]-tableRange[0]);
        break;
      case VTK_STRING:
        // not supported
        assert("check: impossible case" && 0);
        break;
      case VTK_UNSIGNED_SHORT:
        internalFormat=GL_INTENSITY16;
        format=GL_RED;
        type=GL_UNSIGNED_SHORT;

        shift=-tableRange[0]/VTK_UNSIGNED_SHORT_MAX;
        scale=
          VTK_UNSIGNED_SHORT_MAX/(tableRange[1]-tableRange[0]);
        break;
      case VTK_UNSIGNED_INT:
        internalFormat=GL_INTENSITY16;
        format=GL_RED;
        type=GL_UNSIGNED_INT;

        shift=-tableRange[0]/VTK_UNSIGNED_INT_MAX;
        scale=VTK_UNSIGNED_INT_MAX/(tableRange[1]-tableRange[0]);
        break;
      default:
        assert("check: impossible case" && 0);
        break;
      }
    }

  int* ext = imageData->GetExtent();
  this->TextureExtents[0] = ext[0];
  this->TextureExtents[1] = ext[1];
  this->TextureExtents[2] = ext[2];
  this->TextureExtents[3] = ext[3];
  this->TextureExtents[4] = ext[4];
  this->TextureExtents[5] = ext[5];

  std::cerr << "TextureExtents "
            << this->TextureExtents[0] << " "
            << this->TextureExtents[1] << " "
            << this->TextureExtents[2] << " "
            << this->TextureExtents[3] << " "
            << this->TextureExtents[4] << " "
            << this->TextureExtents[5] << std::endl;

  void* dataPtr = scalars->GetVoidPointer(0);
  int i = 0;
  while(i < 3)
    {
    this->TextureSize[i] = this->TextureExtents[2*i+1]-this->TextureExtents[2*i]+1;
    ++i;
    }

  std::cerr << "TextureSize "
            << TextureSize[0] << " "
            << TextureSize[1] << " "
            << TextureSize[2] << std::endl;

  glTexImage3D(GL_TEXTURE_3D, 0, internalFormat,
               this->TextureSize[0],this->TextureSize[1],this->TextureSize[2], 0,
               format, type, dataPtr);

  GL_CHECK_ERRORS

  // Generate mipmaps
  glGenerateMipmap(GL_TEXTURE_3D);

  this->VolmeLoaded = true;
  return this->VolmeLoaded;
}


bool vtkSinglePassSimpleVolumeMapper::vtkInternal::IsVolmeLoaded()
{
  return this->VolmeLoaded;
}


bool vtkSinglePassSimpleVolumeMapper::vtkInternal::IsInitialized()
{
  return this->Initialized;
}


vtkSinglePassSimpleVolumeMapper::vtkSinglePassSimpleVolumeMapper() : vtkVolumeMapper()
{
  this->Implementation = new vtkInternal(this);
}


vtkSinglePassSimpleVolumeMapper::~vtkSinglePassSimpleVolumeMapper()
{
  delete this->Implementation;
  this->Implementation = 0;
}


void vtkSinglePassSimpleVolumeMapper::PrintSelf(ostream &os, vtkIndent indent)
{
  // TODO Implement this method
}


void vtkSinglePassSimpleVolumeMapper::Render(vtkRenderer* ren, vtkVolume* vol)
{
  vtkImageData* input = this->GetInput();

  glClear(GL_DEPTH_BUFFER_BIT);
  glClearColor(1.0, 1.0, 1.0, 1.0);

  if (!this->Implementation->IsInitialized())
    {
    std::cerr << this->Implementation->IsInitialized() << std::endl;

    GLenum err = glewInit();
    if (GLEW_OK != err)	{
        cerr<<"Error: "<<glewGetErrorString(err)<<endl;
    } else {
        if (GLEW_VERSION_3_3)
        {
            cout<<"Driver supports OpenGL 3.3\nDetails:"<<endl;
        }
    }
    err = glGetError(); //this is to ignore INVALID ENUM error 1282
    GL_CHECK_ERRORS

    //output hardware information
    cout<<"\tUsing GLEW "<< glewGetString(GLEW_VERSION)<<endl;
    cout<<"\tVendor: "<< glGetString (GL_VENDOR)<<endl;
    cout<<"\tRenderer: "<< glGetString (GL_RENDERER)<<endl;
    cout<<"\tVersion: "<< glGetString (GL_VERSION)<<endl;
    cout<<"\tGLSL: "<< glGetString (GL_SHADING_LANGUAGE_VERSION)<<endl;
    }

  if (!this->Implementation->IsVolmeLoaded())
    {
    this->Implementation->LoadVolume(input);
    this->Implementation->ComputeTransferFunction();
    }

  if (!this->Implementation->IsInitialized())
    {
    // Load the raycasting shader
    this->Implementation->shader.LoadFromFile(GL_VERTEX_SHADER, "shaders/raycaster.vert");
    this->Implementation->shader.LoadFromFile(GL_FRAGMENT_SHADER, "shaders/raycaster.frag");

    // Compile and link the shader
    this->Implementation->shader.CreateAndLinkProgram();
    this->Implementation->shader.Use();
        //add attributes and uniforms
        this->Implementation->shader.AddAttribute("vVertex");
        this->Implementation->shader.AddUniform("MVP");
        this->Implementation->shader.AddUniform("volume");
        this->Implementation->shader.AddUniform("camPos");
        this->Implementation->shader.AddUniform("step_size");
        this->Implementation->shader.AddUniform("transfer_func");

        //pass constant uniforms at initialization
        glUniform3f(this->Implementation->shader("step_size"),
                    1.0f/(this->Implementation->TextureSize[0] * 10),
                    1.0f/(this->Implementation->TextureSize[1] * 10),
                    1.0f/(this->Implementation->TextureSize[2] * 10));
        glUniform1i(this->Implementation->shader("volume"),0);
        glUniform1i(this->Implementation->shader("transfer_func"), 1);

        // Bind textures
        glEnable(GL_TEXTURE_3D);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, this->Implementation->GetVolumeTexture());

        glEnable(GL_TEXTURE_1D);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_1D, this->Implementation->GetTransferFunctionSampler());
    this->Implementation->shader.UnUse();

    GL_CHECK_ERRORS

    // Setup unit cube vertex array and vertex buffer objects
    glGenVertexArrays(1, &this->Implementation->cubeVAOID);
    glGenBuffers(1, &this->Implementation->cubeVBOID);
    glGenBuffers(1, &this->Implementation->cubeIndicesID);

    // Unit cube vertices (hard coded now)
    float vertices[8][3] = {{-0.5f,-0.5f,-0.5f},
                            { 0.5f,-0.5f,-0.5f},
                            { 0.5f, 0.5f,-0.5f},
                            {-0.5f, 0.5f,-0.5f},
                            {-0.5f,-0.5f, 0.5f},
                            { 0.5f,-0.5f, 0.5f},
                            { 0.5f, 0.5f, 0.5f},
                            {-0.5f, 0.5f, 0.5f}};

    // Unit cube indices
    GLushort cubeIndices[36]={0,5,4,
                              5,0,1,
                              3,7,6,
                              3,6,2,
                              7,4,6,
                              6,4,5,
                              2,1,3,
                              3,1,0,
                              3,0,7,
                              7,0,4,
                              6,5,2,
                              2,5,1};

    glBindVertexArray(this->Implementation->cubeVAOID);
    glBindBuffer (GL_ARRAY_BUFFER, this->Implementation->cubeVBOID);

    // pass cube vertices to buffer object memory
    glBufferData (GL_ARRAY_BUFFER, sizeof(vertices), &(vertices[0][0]), GL_STATIC_DRAW);

    GL_CHECK_ERRORS

    //enable vertex attributre array for position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,0,0);

    //pass indices to element array  buffer
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, this->Implementation->cubeIndicesID);
    glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), &cubeIndices[0], GL_STATIC_DRAW);

    glBindVertexArray(0);

    //enable depth test
    glEnable(GL_DEPTH_TEST);

    //set the over blending function
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    cout<<"Initialization successfull"<<endl;

    this->Implementation->Initialized = true;
    }

  // Enable blending
  glEnable(GL_BLEND);
  glBindVertexArray(this->Implementation->cubeVAOID);

  // Use the shader
  this->Implementation->shader.Use();

  GL_CHECK_ERRORS

  // Pass shader uniforms
  double* tmpPos = ren->GetActiveCamera()->GetPosition();
  float pos[3] = {tmpPos[0], tmpPos[1], tmpPos[2]};

  // Look at the OpenGL Camera for the exact aspect computation
  double aspect[2];
  ren->ComputeAspect();
  ren->GetAspect(aspect);

  vtkMatrix4x4* projMat = ren->GetActiveCamera()->
    GetCompositeProjectionTransformMatrix(aspect[0], -1, 1);
  projMat->Transpose();

  float mvp[16];
  for (int i = 0; i < 4; ++i)
    {
    for (int j = 0; j < 4; ++j)
      {
      mvp[i * 4 + j] = projMat->Element[i][j];
      }
    }

  glUniformMatrix4fv(this->Implementation->shader("MVP"), 1, GL_FALSE, &(mvp[0]));
  glUniform3fv(this->Implementation->shader("camPos"), 1, &(pos[0]));
  glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);

  this->Implementation->shader.UnUse();

  glDisable(GL_BLEND);
}
