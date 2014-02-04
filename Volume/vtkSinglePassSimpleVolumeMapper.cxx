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

#include <GL/glew.h>
#include <vtkgl.h>

#include <cassert>

vtkStandardNewMacro(vtkSinglePassSimpleVolumeMapper);

// Remove this afterwards
#define GL_CHECK_ERRORS assert(glGetError()== GL_NO_ERROR);

// Class that implements control point for the transfer function
class TransferControlPoint
{
public:
    float Color[4];
    int IsoValue;

    void TransferControlPoint(float r, float g, float b, int isovalue)
    {
      Color.X = r;
      Color.Y = g;
      Color.Z = b;
      Color.W = 1.0f;
      IsoValue = isovalue;
    }

    void TransferControlPoint(float alpha, int isovalue)
    {
      Color.X = 0.0f;
      Color.Y = 0.0f;
      Color.Z = 0.0f;
      Color.W = alpha;
      IsoValue = isovalue;
    }
};

// Class that hides implementation details
class vtkSinglePassSimpleVolumeMapper::vtkInternal
{
public:
  vtkInternal(vtkSinglePassSimpleVolumeMapper* parent) :
    Parent(parent),
    VolmeLoaded(false), Initialized(false)
    {
    }

  ~vtkInternal()
    {
    }

  bool LoadVolume(vtkImageData* imageData);
  bool IsVolmeLoaded();
  bool IsInitialized();
  void ComputeTransferFunction();

  vtkSinglePassSimpleVolumeMapper* Parent;

  bool VolmeLoaded;
  bool Initialized;

  GLuint cubeVBOID;
  GLuint cubeVAOID;
  GLuint cubeIndicesID;
  GLSLShader shader;

  GLuint TransferFunc;

  int CellFlag;
  int TextureSize[3];
  int TextureExtents[6];

  std::vector<TransferControlPoint> ColorKnots;
  std::vector<TransferControlPoint> AlphaKnots;
};

// Helper method for computing the transfer function
void vtkSinglePassSimpleVolumeMapper::vtkInternal::ComputeTransferFunction()
{
  // Initialize the cubic spline for the transfer function
  float transferFunc[256*4];

  // Fit a cubic spline for the transfer function
  std::vector<Cubic> colorCubic = this->CalculateCubicSpline(ColorKnots.size() - 1, ColorKnots);
  std::vector<Cubic> alphaCubic = this->CalculateCubicSpline(AlphaKnots.size() - 1, AlphaKnots);

  int numTF = 0;
  for (int i = 0; i < ColorKnots.size() - 1; i++)
    {
    // Compute steps
    int steps = mColorKnots[i + 1].IsoValue - mColorKnots[i].IsoValue;

    // Now compute the color
    for (int j = 0; j < steps; j++)
      {
      float k = (float)j / (float)(steps - 1);

      std::vector<float> color = colorCubic[i].GetPointOnSpline(k);
      for (int k = 0; k < 4; ++k)
        {
        transferFunc[numTF++] = color[k];
        }
      }
    }

  numTF = 0;
  for (int i = 0; i < AlphaKnots.Count - 1; i++)
    {
    int steps = AlphaKnots[i + 1].IsoValue - AlphaKnots[i].IsoValue;
    for (int j = 0; j < steps; j++)
      {
      float k = (float)j / (float)(steps - 1);

      std::vector<float> color = colorCubic[i].GetPointOnSpline(k);
      transferFunc[numTF + (j+1) * 3 + 1] = color[3];
    }

  // Generate OpenGL texture
  glGenTextures(1, &this->TransferFunc);
  glBindTexture(GL_TEXTURE_1D, this->TransferFunc);

  // Set the texture parameters
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_R, GL_CLAMP);


}


bool vtkSinglePassSimpleVolumeMapper::vtkInternal::LoadVolume(vtkImageData* imageData)
{
  //volume texture ID
  GLuint textureID;

  // generate OpenGL texture
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_3D, textureID);

  // set the texture parameters
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

  //set the mipmap levels (base and max)
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 4);

  GL_CHECK_ERRORS

  //allocate data with internal format and foramt as (GL_RED)
  GLint internalFormat=0;
  GLenum format=0;
  GLenum type=0;

  // @aashish: tableRange is for transform function
  double tableRange[2];

  double shift=0.0;
  double scale=1.0;
  int needTypeConversion=0;

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

  glTexImage3D(GL_TEXTURE_3D,0,internalFormat,
               this->TextureSize[0],this->TextureSize[1],this->TextureSize[2],0,
               format,type, dataPtr);

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
    }

  if (!this->Implementation->IsInitialized())
    {
    // Load the raycasting shader
    this->Implementation->shader.LoadFromFile(GL_VERTEX_SHADER, "shaders/raycaster.vert");
    this->Implementation->shader.LoadFromFile(GL_FRAGMENT_SHADER, "shaders/raycaster.frag");

    //compile and link the shader
    this->Implementation->shader.CreateAndLinkProgram();
    this->Implementation->shader.Use();

        //add attributes and uniforms
        this->Implementation->shader.AddAttribute("vVertex");
        this->Implementation->shader.AddUniform("MVP");
        this->Implementation->shader.AddUniform("volume");
        this->Implementation->shader.AddUniform("camPos");
        this->Implementation->shader.AddUniform("step_size");

        //pass constant uniforms at initialization
        glUniform3f(this->Implementation->shader("step_size"),
                    1.0f/(this->Implementation->TextureSize[0] * 10),
                    1.0f/(this->Implementation->TextureSize[1] * 10),
                    1.0f/(this->Implementation->TextureSize[2] * 10));
        glUniform1i(this->Implementation->shader("volume"),0);
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
