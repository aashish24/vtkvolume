#version 330 core

// Inputs
//
//////////////////////////////////////////////////////////////////////////////

/// Object space vertex position
layout(location = 0) in vec3 vVertex;

// Uniforms
//
//////////////////////////////////////////////////////////////////////////////

/// combined modelview projection matrix
uniform mat4 MVP;

uniform vec3 vol_extents_min;
uniform vec3 vol_extents_max;

// Outputs
//
//////////////////////////////////////////////////////////////////////////////

/// 3D texture coordinates for texture lookup in the fragment shader
smooth out vec3 texture_coords;
smooth out vec3 vertex_pos;

void main()
{
  /// Get clipspace position
  vec4 pos = MVP * vec4(vVertex.xyz, 1);
  gl_Position = pos;
  vertex_pos = vVertex;
  vec3 uv = (vVertex - vol_extents_min) / (vol_extents_max - vol_extents_min);
  texture_coords = uv;
}
