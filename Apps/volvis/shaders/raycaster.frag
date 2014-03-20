#version 330 core

/// Outputs
///
//////////////////////////////////////////////////////////////////////////////

/// Fragment shader output
layout(location = 0) out vec4 dst;

/// 3D texture coordinates form vertex shader
smooth in vec3 texture_coords;

smooth in vec3 vertex_pos;

/// Uniforms
///
//////////////////////////////////////////////////////////////////////////////

/// Volume dataset
uniform sampler3D	volume;

/// Transfer functions
uniform sampler1D color_transfer_func;
uniform sampler1D opacity_transfer_func;

/// Camera position
uniform vec3 camera_pos;

/// Ray step size
uniform vec3 step_size;

/// Enable / disable shading
uniform bool enable_shading;

/// Material and lighting
uniform vec3 diffuse;
uniform vec3 ambient;
uniform vec3 specular;
uniform vec4 light_position;
uniform float shininess;

/// Constants
///
//////////////////////////////////////////////////////////////////////////////

/// Total samples for each ray march step
const int MAX_SAMPLES = 300;

/// Minimum texture access coordinate
const vec3 texMin = vec3(0);

/// Maximum texture access coordinate
const vec3 texMax = vec3(1);

/// Globals
vec3 dataPos;

const vec4 clampMin = vec4(0.0);
const vec4 clampMax = vec4(1.0);

/// Perform shading on the volume
///
///
//////////////////////////////////////////////////////////////////////////////
void shade()
{
  /// g1 - g2 is gradient in texture coordinates
  vec3 g1;
  vec3 g2;

  g1.x = texture3D(volume, vec3 dataPos + step_size[0]).x;
  g1.y = texture3D(volume, vec3 dataPos + step_size[1]).x;
  g1.z = texture3D(volume, vec3 dataPos + step_size[2]).x;

  g2.x = texture3D(volume, vec3 dataPos - step_size[0]).x;
  g2.y = texture3D(volume, vec3 dataPos - step_size[1]).x;
  g2.z = texture3D(volume, vec3 dataPos - step_size[2]).x;

  g2 = g1 - g2;
  g2 = g2 * cell_scale;

  float normalLength = length(g2);
  if(normalLength > 0.0) {
    /// TODO Implement this
    //g2 = normalize(transposeTextureToEye * g2);
    g2 = normalize(g2);
  } else {
    g2 = vec3(0.0,0.0,0.0);
  }

  /// TODO Implement this
  vec4 color = colorFromValue(value);

  /// initialize color to 0.0
  vec4 finalColor = vec4(1.0, 1.0, 1.0, 1.0);

  /// TODO Making assumption that light position is same as camera position
  ///      Also, using directional light for now
  light_position = vec4(camera_pos, 0.0);

  /// Perform simple light calculations
  float nDotL = dot(g2, ldir);
  float nDotH = dot(g2, h);

  /// Separate nDotL and nDotH for two-sided shading, otherwise we
  /// get black spots.

  /// Two-sided shading
  if (nDotL < 0.0) {
    nDotL =- nDotL;
  }

  /// Two-sided shading
  if (nDotH < 0.0) {
    nDotH =- nDotH;
  }

  /// Ambient term for this light
  finalColor += ambient;

  /// Diffuse term for this light
  if(nDotL > 0.0) {
    finalColor += diffuse * nDotL * color;
  }

  /// Specular term for this light
  float shininessFactor = pow(nDotH, shininess);
  finalColor += specular * shininessFactor;

  /// clamp values otherwise we get black spots
  finalColor = clamp(finalColor,clampMin,clampMax);

  return finalColor;
}

/// Main
///
//////////////////////////////////////////////////////////////////////////////
void main()
{
  /// Get the 3D texture coordinates for lookup into the volume dataset
  dataPos = texture_coords.xyz;

  /// For now assume identity scene matrix
  mat4 inv_model_matrix = mat4(1);

  /// Getting the ray marching direction (in object space);
  vec3 geomDir = normalize(vertex_pos.xyz - vec4(inv_model_matrix * vec4(camera_pos, 1.0)).xyz);

  /// Multiply the raymarching direction with the step size to get the
  /// sub-step size we need to take at each raymarching step
  vec3 dirStep = geomDir * step_size;

  /// Flag to indicate if the raymarch loop should terminate
  bool stop = false;

  /// For all samples along the ray
  for (int i = 0; i < MAX_SAMPLES; i++) {
    /// Advance ray by dirstep
    dataPos = dataPos + dirStep;

    /// The two constants texMin and texMax have a value of vec3(-1,-1,-1)
    /// and vec3(1,1,1) respectively. To determine if the data value is
    /// outside the volume data, we use the sign function. The sign function
    /// return -1 if the value is less than 0, 0 if the value is equal to 0
    /// and 1 if value is greater than 0. Hence, the sign function for the
    /// calculation (sign(dataPos-texMin) and sign (texMax-dataPos)) will
    /// give us vec3(1,1,1) at the possible minimum and maximum position.
    /// When we do a dot product between two vec3(1,1,1) we get the answer 3.
    /// So to be within the dataset limits, the dot product will return a
    /// value less than 3. If it is greater than 3, we are already out of
    /// the volume dataset
    stop = dot(sign(dataPos - texMin), sign(texMax - dataPos)) < 3.0;

    //if the stopping condition is true we brek out of the ray marching loop
    if (stop)
      break;

    /// Data fetching from the red channel of volume texture
    float scalar = texture(volume, dataPos).r;
    vec4 src = vec4(texture(color_transfer_func, scalar).xyz,
                    texture(opacity_transfer_func, scalar).w);

    /// Opacity calculation using compositing:
    /// here we use front to back compositing scheme whereby the current sample
    /// value is multiplied to the currently accumulated alpha and then this product
    /// is subtracted from the sample value to get the alpha from the previous steps.
    /// Next, this alpha is multiplied with the current sample colour and accumulated
    /// to the composited colour. The alpha value from the previous steps is then
    /// accumulated to the composited colour alpha.
    src.rgb *= src.a;
    dst = (1.0f - dst.a) * src + dst;

    /// Early ray termination
    /// if the currently composited colour alpha is already fully saturated
    /// we terminated the loop
    if(dst.a > 0.99)
      break;
  }
}
