#version 330 core

// Object space vertex position
layout(location = 0) in vec3 vVertex;

layout(location = 1) in vec2 vUV;

// Uniforms
// combined modelview projection matrix
uniform mat4 MVP;

// 3D texture coordinates for texture lookup in the fragment shader
smooth out vec3 vUVOut;

void main()
{
  //get the clipspace position
  gl_Position = MVP * vec4(vVertex.xyz,1);

  vUVOut = vec3(vUV, 1.0);
}
