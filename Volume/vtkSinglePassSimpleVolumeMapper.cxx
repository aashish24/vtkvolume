#include "vtkSinglePassSimpleVolumeMapper.h"
#include "vtkTransferControlPoint.h"
#include "vtkCubic.h"

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
#define GL_CHECK_ERRORS \
  {\
  std::cerr << "Checking for error " << glGetError() << std::endl; \
  assert(glGetError()== GL_NO_ERROR); \
  }

// Class that hides implementation details
class vtkSinglePassSimpleVolumeMapper::vtkInternal
{
public:
  vtkInternal(vtkSinglePassSimpleVolumeMapper* parent) :
    Parent(parent),
    VolmeLoaded(false), Initialized(false), ValidTransferFunction(false)
    {
    this->ColorKnots = std::vector<vtkTransferControlPoint>();
    this->ColorKnots.push_back(vtkTransferControlPoint(0.0, 0.0, 0.0, 0));
    this->ColorKnots.push_back(vtkTransferControlPoint(0.0, 0.0, 0.0, 128));
    this->ColorKnots.push_back(vtkTransferControlPoint(0.0, 1.0, 0.0, 256));
    this->ColorKnots.push_back(vtkTransferControlPoint(0.0, 1.0, 0.0, 512));

    this->AlphaKnots = std::vector<vtkTransferControlPoint>();
    this->AlphaKnots.push_back(vtkTransferControlPoint(0.0f, 0));
    this->AlphaKnots.push_back(vtkTransferControlPoint(0.025f, 50));
    this->AlphaKnots.push_back(vtkTransferControlPoint(0.05f, 100));
    this->AlphaKnots.push_back(vtkTransferControlPoint(0.1f, 256));
    this->AlphaKnots.push_back(vtkTransferControlPoint(0.2f, 512));
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
  GLuint CubeIndicesID;
  GLuint CubeTextureID;

  GLSLShader shader;

  GLuint VolumeTexture;
  GLuint TransferFuncSampler;

  int CellFlag;
  int TextureSize[3];
  int TextureExtents[6];

  std::vector<vtkTransferControlPoint> ColorKnots;
  std::vector<vtkTransferControlPoint> AlphaKnots;
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
  std::vector<vtkCubic> colorCubic = vtkCubic::CalculateCubicSpline(ColorKnots.size() - 1, ColorKnots);
  std::vector<vtkCubic> alphaCubic = vtkCubic::CalculateCubicSpline(AlphaKnots.size() - 1, AlphaKnots);

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

  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  // Set the mipmap levels (base and max)
  //  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
  //  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 4);

  GL_CHECK_ERRORS

  // Allocate data with internal format and foramt as (GL_RED)
  GLint internalFormat = 0;
  GLenum format = 0;
  GLenum type = 0;

  double shift = 0.0;
  double scale = 1.0;
  int needTypeConversion = 0;

 // @aashish: tableRange is for transform function in existing VTK code.
 double tableRange[2];

  vtkDataArray* scalars = this->Parent->GetScalars(imageData,
                          this->Parent->ScalarMode,
                          this->Parent->ArrayAccessMode,
                          this->Parent->ArrayId,
                          this->Parent->ArrayName,
                          this->CellFlag);

  int scalarType = scalars->GetDataType();
  if(scalars->GetNumberOfComponents()==4)
    {
    // this is RGBA, unsigned char only
    internalFormat = GL_RGBA16;
    format = GL_RGBA;
    type = GL_UNSIGNED_BYTE;
    }
  else
    {
    switch(scalarType)
      {
      case VTK_FLOAT:
        //          if(this->Supports_GL_ARB_texture_float)
        //            {
        //            internalFormat = vtkgl::INTENSITY16F_ARB;
        //            }
        //          else
        //            {
            internalFormat = GL_INTENSITY16;
        //            }
        format = GL_RED;
        type = GL_FLOAT;
        shift=-tableRange[0];
        scale = 1/(tableRange[1]-tableRange[0]);
        break;
      case VTK_UNSIGNED_CHAR:
        internalFormat = GL_INTENSITY8;
        format = GL_RED;
        type = GL_UNSIGNED_BYTE;
        shift = -tableRange[0]/VTK_UNSIGNED_CHAR_MAX;
        scale =
          VTK_UNSIGNED_CHAR_MAX/(tableRange[1]-tableRange[0]);
        break;
      case VTK_SIGNED_CHAR:
        internalFormat = GL_INTENSITY8;
        format = GL_RED;
        type = GL_BYTE;
        shift=-(2*tableRange[0]+1)/VTK_UNSIGNED_CHAR_MAX;
        scale = VTK_SIGNED_CHAR_MAX/(tableRange[1]-tableRange[0]);
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
        internalFormat = GL_INTENSITY16;
        format = GL_RED;
        type = GL_INT;
        shift=-(2*tableRange[0]+1)/VTK_UNSIGNED_INT_MAX;
        scale = VTK_INT_MAX/(tableRange[1]-tableRange[0]);
        break;
      case VTK_DOUBLE:
      case VTK___INT64:
      case VTK_LONG:
      case VTK_LONG_LONG:
      case VTK_UNSIGNED___INT64:
      case VTK_UNSIGNED_LONG:
      case VTK_UNSIGNED_LONG_LONG:
      //          needTypeConversion = 1; // to float
      //          if(this->Supports_GL_ARB_texture_float)
      //            {
      //            internalFormat = vtkgl::INTENSITY16F_ARB;
      //            }
      //          else
      //            {
      //            internalFormat = GL_INTENSITY16;
      //            }
      //          format = GL_RED;
      //          type = GL_FLOAT;
      //          shift=-tableRange[0];
      //          scale = 1/(tableRange[1]-tableRange[0]);
      //          sliceArray = vtkFloatArray::New();
        break;
      case VTK_SHORT:
        internalFormat = GL_INTENSITY16;
        format = GL_RED;
        type = GL_SHORT;
        shift=-(2*tableRange[0]+1)/VTK_UNSIGNED_SHORT_MAX;
        scale = VTK_SHORT_MAX/(tableRange[1]-tableRange[0]);
        break;
      case VTK_STRING:
        // not supported
        assert("check: impossible case" && 0);
        break;
      case VTK_UNSIGNED_SHORT:
        internalFormat = GL_INTENSITY16;
        format = GL_RED;
        type = GL_UNSIGNED_SHORT;

        shift=-tableRange[0]/VTK_UNSIGNED_SHORT_MAX;
        scale=
          VTK_UNSIGNED_SHORT_MAX/(tableRange[1]-tableRange[0]);
        break;
      case VTK_UNSIGNED_INT:
        internalFormat = GL_INTENSITY16;
        format = GL_RED;
        type = GL_UNSIGNED_INT;

        shift=-tableRange[0]/VTK_UNSIGNED_INT_MAX;
        scale = VTK_UNSIGNED_INT_MAX/(tableRange[1]-tableRange[0]);
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
  //glGenerateMipmap(GL_TEXTURE_3D);

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

  double bounds[6];
  vol->GetBounds(bounds);

//  bounds[1] += 1;
//  bounds[3] += 1;
//  bounds[5] += 1;

  std::cerr << "bounds "
            << bounds[0] << " "
            << bounds[1] << " "
            << bounds[2] << " "
            << bounds[3] << " "
            << bounds[4] << " "
            << bounds[5] << " " << std::endl;

  if (!this->Implementation->IsInitialized())
    {
    // Load the raycasting shader
    this->Implementation->shader.LoadFromFile(GL_VERTEX_SHADER, "shaders/raycaster.vert");
    this->Implementation->shader.LoadFromFile(GL_FRAGMENT_SHADER, "shaders/raycaster.frag");

    // Compile and link the shader
    this->Implementation->shader.CreateAndLinkProgram();
    this->Implementation->shader.Use();
        // Add attributes and uniforms
        this->Implementation->shader.AddAttribute("in_vertex_pos");
        this->Implementation->shader.AddUniform("modelview_matrix");
        this->Implementation->shader.AddUniform("projection_matrix");
        this->Implementation->shader.AddUniform("volume");
        this->Implementation->shader.AddUniform("camera_pos");
        this->Implementation->shader.AddUniform("step_size");
        this->Implementation->shader.AddUniform("transfer_func");
        this->Implementation->shader.AddUniform("vol_extents_min");
        this->Implementation->shader.AddUniform("vol_extents_max");

        // Pass constant uniforms at initialization
        // Step should be dependant on the bounds and not on the texture size
        // since we can have non uniform voxel size / spacing / aspect ratio
        glUniform3f(this->Implementation->shader("step_size"),
                    1.0f / (bounds[1] - bounds[0]),
                    1.0f / (bounds[3] - bounds[2]),
                    1.0f / (bounds[5] - bounds[4]));
        glUniform1i(this->Implementation->shader("volume"), 0);
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
    glGenBuffers(1, &this->Implementation->CubeIndicesID);
    glGenBuffers(1, &this->Implementation->CubeTextureID);

//    // Cube vertices
    float vertices[8][3] =
      {
      {bounds[0], bounds[2], bounds[4]}, // 0
      {bounds[1], bounds[2], bounds[4]}, // 1
      {bounds[1], bounds[3], bounds[4]}, // 2
      {bounds[0], bounds[3], bounds[4]}, // 3
      {bounds[0], bounds[2], bounds[5]}, // 4
      {bounds[1], bounds[2], bounds[5]}, // 5
      {bounds[1], bounds[3], bounds[5]}, // 6
      {bounds[0], bounds[3], bounds[5]}  // 7
      };

    // Cube indices
    GLushort cubeIndices[36]=
      {
      0,5,4, // bottom
      5,0,1, // bottom
      3,7,6, // top
      3,6,2, // op
      7,4,6, // front
      6,4,5, // front
      2,1,3, // left side
      3,1,0, // left side
      3,0,7, // right side
      7,0,4, // right side
      6,5,2, // back
      2,5,1  // back
      };

//    float vertices[24][3] =
//      {
//      // Front
//      {bounds[0], bounds[2], bounds[5]},
//      {bounds[1], bounds[2], bounds[5]},
//      {bounds[1], bounds[3], bounds[5]},
//      {bounds[0], bounds[3], bounds[5]},
//      // Right
//      {bounds[1], bounds[2], bounds[5]},
//      {bounds[1], bounds[2], bounds[4]},
//      {bounds[1], bounds[3], bounds[4]},
//      {bounds[1], bounds[3], bounds[5]},
//      // Back
//      {bounds[1], bounds[2], bounds[4]},
//      {bounds[0], bounds[2], bounds[4]},
//      {bounds[0], bounds[3], bounds[4]},
//      {bounds[1], bounds[3], bounds[4]},
//      // Left
//      {bounds[0], bounds[2], bounds[4]},
//      {bounds[0], bounds[2], bounds[5]},
//      {bounds[0], bounds[3], bounds[5]},
//      {bounds[0], bounds[3], bounds[4]},
//      // Bottom
//      {bounds[0], bounds[2], bounds[4]},
//      {bounds[1], bounds[2], bounds[4]},
//      {bounds[1], bounds[2], bounds[5]},
//      {bounds[0], bounds[2], bounds[5]},
//      // Top
//      {bounds[0], bounds[3], bounds[5]},
//      {bounds[1], bounds[3], bounds[5]},
//      {bounds[1], bounds[3], bounds[4]},
//      {bounds[0], bounds[3], bounds[4]}
//    };

//    // Cube indices
//    GLushort cubeIndices[36]=
//      {
//      0, 1, 2,
//      0, 2, 3,
//      4, 5, 6,
//      4, 6, 7,
//      8, 9, 10,
//      8, 10, 11,
//      12, 13, 14,
//      12, 14, 15,
//      16, 17, 18,
//      16, 18, 19,
//      20, 21, 22,
//      20, 22, 23
//      };

    glBindVertexArray(this->Implementation->cubeVAOID);

    // pass cube vertices to buffer object memory
    glBindBuffer (GL_ARRAY_BUFFER, this->Implementation->cubeVBOID);
    glBufferData (GL_ARRAY_BUFFER, sizeof(vertices), &(vertices[0][0]), GL_STATIC_DRAW);

    GL_CHECK_ERRORS

    // Enable vertex attributre array for position
    // and pass indices to element array  buffer
    glEnableVertexAttribArray(this->Implementation->shader["in_vertex_pos"]);
    glVertexAttribPointer(this->Implementation->shader["in_vertex_pos"], 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, this->Implementation->CubeIndicesID);
    glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), &cubeIndices[0], GL_STATIC_DRAW);

    GL_CHECK_ERRORS

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

  // Use the shader
  this->Implementation->shader.Use();

  GL_CHECK_ERRORS

  // Pass shader uniforms
  double* tmpPos = ren->GetActiveCamera()->GetPosition();
  float pos[3] = {tmpPos[0], tmpPos[1], tmpPos[2]};

  std::cerr << "Camera position " << pos[0] << " " << pos[1] << " " << pos[2] << std::endl;

  // Look at the OpenGL Camera for the exact aspect computation
  double aspect[2];
  ren->ComputeAspect();
  ren->GetAspect(aspect);

  double clippingRange[2];
  ren->GetActiveCamera()->GetClippingRange(clippingRange);

  // Will require transpose of this matrix for OpenGL
  // Fix this
  vtkMatrix4x4* projMat = ren->GetActiveCamera()->
    GetProjectionTransformMatrix(aspect[0]/aspect[1], 0, 1);
  float projectionMat[16];
  for (int i = 0; i < 4; ++i)
    {
    for (int j = 0; j < 4; ++j)
      {
      projectionMat[i * 4 + j] = projMat->Element[i][j];
      }
    }

  // Will require transpose of this matrix for OpenGL
  // Fix this
  vtkMatrix4x4* mvMat = ren->GetActiveCamera()->GetViewTransformMatrix();
  float modelviewMat[16];
  for (int i = 0; i < 4; ++i)
    {
    for (int j = 0; j < 4; ++j)
      {
      modelviewMat[i * 4 + j] = mvMat->Element[i][j];
      }
    }

  glUniformMatrix4fv(this->Implementation->shader("projection_matrix"), 1, GL_FALSE, &(projectionMat[0]));
  glUniformMatrix4fv(this->Implementation->shader("modelview_matrix"), 1, GL_FALSE, &(modelviewMat[0]));

  glUniform3fv(this->Implementation->shader("camera_pos"), 1, &(pos[0]));

  float volExtentsMin[3] = {bounds[0], bounds[2], bounds[4]};
  float volExtentsMax[3] = {bounds[1], bounds[3], bounds[5]};

  glUniform3fv(this->Implementation->shader("vol_extents_min"), 1, &(volExtentsMin[0]));
  glUniform3fv(this->Implementation->shader("vol_extents_max"), 1, &(volExtentsMax[0]));

  glBindVertexArray(this->Implementation->cubeVAOID);

  glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);

  this->Implementation->shader.UnUse();

  glDisable(GL_BLEND);
}
