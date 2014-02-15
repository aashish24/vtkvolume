#version 330 core

// Object space vertex position
layout(location = 0) in vec3 vVertex;

layout(location = 1) in vec3 vUV;

// Uniforms
// combined modelview projection matrix
uniform mat4 MVP;

uniform vec3 vol_extents_min;
uniform vec3 vol_extents_max;

// 3D texture coordinates for texture lookup in the fragment shader
smooth out vec3 vUVOut;

void main()
{
  //get the clipspace position
  gl_Position = MVP * vec4(vVertex.xyz, 1);

  //vUVOut = vUV;
  vec3 uv = (vVertex - vol_extents_min) / (vol_extents_max - vol_extents_min);
  vUVOut = uv;
}
