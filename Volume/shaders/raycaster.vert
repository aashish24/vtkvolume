#version 330 core

// Object space vertex position
layout(location = 0) in vec3 vVertex;

// Object texture coordinates
in vec2 textureCoords;

// Uniforms

// combined modelview projection matrix
uniform mat4 MVP;

// 3D texture coordinates for texture lookup in the fragment shader
smooth out vec3 vUV;

void main()
{
  //get the clipspace position
  gl_Position = MVP*vec4(vVertex.xyz,1);

  vUV = vec3(textureCoords, 1.0);
}
