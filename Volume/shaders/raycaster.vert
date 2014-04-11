#version 330 core

// Inputs
//
//////////////////////////////////////////////////////////////////////////////

/// Object space vertex position
layout(location = 0) in vec3 in_vertex_pos;

// Uniforms
//
//////////////////////////////////////////////////////////////////////////////

/// combined modelview projection matrix
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform mat4 scene_matrix;

uniform vec3 vol_extents_min;
uniform vec3 vol_extents_max;

// Outputs
//
//////////////////////////////////////////////////////////////////////////////

/// 3D texture coordinates for texture lookup in the fragment shader
out vec3 texture_coords;
smooth out vec3 vertex_pos;

void main()
{
  /// Get clipspace position
  mat4 ogl_projection_matrix = transpose(projection_matrix);
  mat4 ogl_modelview_matrix = transpose(modelview_matrix);
  vec4 pos = ogl_projection_matrix * ogl_modelview_matrix * scene_matrix *
             vec4(in_vertex_pos.xyz, 1);
  gl_Position = pos;
  vertex_pos = in_vertex_pos;

  /// Compute texture coordinates
  vec3 uv = (in_vertex_pos - vol_extents_min) / (vol_extents_max - vol_extents_min);
  texture_coords = uv;
}
