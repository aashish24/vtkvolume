#include "vtkSinglePassSimpleVolumeMapper.h"

#include "GLSLShader.h"

#include <vtkObjectFactory.h>
#include <vtkCamera.h>
#include <vtkMatrix4x4.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

#include <GL/glew.h>
#include <vtkgl.h>

#include <cassert>

vtkStandardNewMacro(vtkSinglePassSimpleVolumeMapper);

// Remove this afterwards
#define GL_CHECK_ERRORS assert(glGetError()== GL_NO_ERROR);

// Class that hides implementation details
class vtkSinglePassSimpleVolumeMapper::vtkInternal
{
public:
  vtkInternal() : VolmeLoaded(false), Initialized(false),
    XDIM(256), YDIM(256), ZDIM(256)
    {
    }

  ~vtkInternal()
    {
    }

  bool LoadVolume();
  bool IsVolmeLoaded();
  bool IsInitialized();

  bool VolmeLoaded;
  bool Initialized;

  GLuint cubeVBOID;
  GLuint cubeVAOID;
  GLuint cubeIndicesID;
  GLSLShader shader;

  int XDIM;
  int YDIM;
  int ZDIM;
};


bool vtkSinglePassSimpleVolumeMapper::vtkInternal::LoadVolume() {

  //volume texture ID
  GLuint textureID;

  const char* volume_file = "media/Engine256.raw";

  std::ifstream infile(volume_file, std::ios_base::binary);
  if(infile.good())
    {
    // read the volume data file
    GLubyte* pData = new GLubyte[this->XDIM*this->YDIM*this->ZDIM];
    infile.read(reinterpret_cast<char*>(pData), this->XDIM*this->YDIM*this->ZDIM*sizeof(GLubyte));
    infile.close();

    //generate OpenGL texture
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

    //allocate data with internal format and foramt as (GL_RED)
    glTexImage3D(GL_TEXTURE_3D,0,GL_RED,this->XDIM,this->YDIM,this->ZDIM,0,GL_RED,GL_UNSIGNED_BYTE,pData);
    GL_CHECK_ERRORS

    //generate mipmaps
    //glGenerateMipmap(GL_TEXTURE_3D);

    // delete the volume data allocated on heap
    delete [] pData;

    this->VolmeLoaded = true;
    }
  else
    {
    std::cerr << "Failed to load the volume file" << std::endl;
    }

  return this->VolmeLoaded;
}


bool vtkSinglePassSimpleVolumeMapper::vtkInternal::IsVolmeLoaded()
{
  // TODO remove this
  return true;


  return this->VolmeLoaded;
}


bool vtkSinglePassSimpleVolumeMapper::vtkInternal::IsInitialized()
{
  return this->Initialized;
}


vtkSinglePassSimpleVolumeMapper::vtkSinglePassSimpleVolumeMapper() : vtkVolumeMapper()
{
  this->Implementation = new vtkInternal();
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
  glClearColor(1.0, 1.0, 1.0, 1.0);

  if (this->Implementation->IsVolmeLoaded())
    {
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
      cout<<"\tUsing GLEW "<<glewGetString(GLEW_VERSION)<<endl;
      cout<<"\tVendor: "<<glGetString (GL_VENDOR)<<endl;
      cout<<"\tRenderer: "<<glGetString (GL_RENDERER)<<endl;
      cout<<"\tVersion: "<<glGetString (GL_VERSION)<<endl;
      cout<<"\tGLSL: "<<glGetString (GL_SHADING_LANGUAGE_VERSION)<<endl;

      // Load the raycasting shader
      this->Implementation->shader.LoadFromFile(GL_VERTEX_SHADER, "shaders/shader.vert");
      this->Implementation->shader.LoadFromFile(GL_FRAGMENT_SHADER, "shaders/shader.frag");

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
                      1.0f/this->Implementation->XDIM,
                      1.0f/this->Implementation->YDIM,
                      1.0f/this->Implementation->ZDIM);
          glUniform1i(this->Implementation->shader("volume"),0);
      this->Implementation->shader.UnUse();

      GL_CHECK_ERRORS

      // Setup unit cube vertex array and vertex buffer objects
      glGenVertexArrays(1, &this->Implementation->cubeVAOID);
      glGenBuffers(1, &this->Implementation->cubeVBOID);
      glGenBuffers(1, &this->Implementation->cubeIndicesID);

      // Unit cube vertices (hard coded now)
      float vertices[8][3] = {{-0.1f,-0.1f,-0.1f},
                              { 0.1f,-0.1f,-0.1f},
                              { 0.1f, 0.1f,-0.1f},
                              {-0.1f, 0.1f,-0.1f},
                              {-0.1f,-0.1f, 0.1f},
                              { 0.1f,-0.1f, 0.1f},
                              { 0.1f, 0.1f, 0.1f},
                              {-0.1f, 0.1f, 0.1f}};

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

//    glEnable(GL_BLEND);
    glBindVertexArray(this->Implementation->cubeVAOID);

    this->Implementation->shader.Use();

    GL_CHECK_ERRORS

    // pass shader uniforms
    double* tmpPos = ren->GetActiveCamera()->GetPosition();
    float pos[3] = {tmpPos[0], tmpPos[1], tmpPos[2]};

    // Look at the OpenGL Camera for the exact aspect computation
    double aspect[2];
    ren->ComputeAspect();
    ren->GetAspect(aspect);

    vtkMatrix4x4* projMat = ren->GetActiveCamera()->GetCompositeProjectionTransformMatrix(aspect[0], -1, 1);
    projMat->Transpose();

//    std::cerr << "proj " << std::endl;
//    std::cerr << "====================== " << std::endl;
//    projMat->PrintSelf(std::cout, vtkIndent());

    float mvp[16];
    for (int i = 0; i < 4; ++i)
      {
      for (int j = 0; j < 4; ++j)
        {
        mvp[i * 4 + j] = projMat->Element[i][j];
        }
      }

//    std::cerr << "this->Implementation->shader(MVP) " << this->Implementation->shader("MVP") << std::endl;
//    std::cerr << "mvProjMat->Element[0][0] " << mvProjMat->Element[0][0] << std::endl;
//    std::cerr << "pos " << pos[0] << " " << pos[1] << " " << pos[2] << std::endl;
//    std::cerr << "foc " << tmpfoc[0] << " " << tmpfoc[1] << " " << tmpfoc[2] << std::endl;

//    mvp[0] = 0.642788;
//    mvp[1] = 0;
//    mvp[2] = 0.766044;
//    mvp[3] = 0;
//    mvp[4] = 0.0534366;
//    mvp[5] = 0.997564;
//    mvp[6] = -0.0448386;
//    mvp[7] = 0;
//    mvp[8] = -0.764178;
//    mvp[9] = 0.0697565;
//    mvp[10] = 0.641222;
//    mvp[11] = -2;
//    mvp[12] = 0;
//    mvp[13] = 0;
//    mvp[14] = 0;
//    mvp[15] = 1;

    glUniformMatrix4fv(this->Implementation->shader("MVP"), 1, GL_FALSE, &(mvp[0]));
    glUniform3fv(this->Implementation->shader("camPos"), 1, &(pos[0]));
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);

    this->Implementation->shader.UnUse();

//    glDisable(GL_BLEND);
  }
}
