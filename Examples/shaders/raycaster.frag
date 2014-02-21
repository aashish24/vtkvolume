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

/// Transfer function
uniform sampler1D transfer_func;

/// Camera position
uniform vec3 camPos;

/// Ray step size
uniform vec3 step_size;

/// Constants
///
//////////////////////////////////////////////////////////////////////////////

/// Total samples for each ray march step
const int MAX_SAMPLES = 300;

/// Minimum texture access coordinate
const vec3 texMin = vec3(0);

/// Maximum texture access coordinate
const vec3 texMax = vec3(1);

/// Main
///
//////////////////////////////////////////////////////////////////////////////
void main()
{
  // Get the 3D texture coordinates for lookup into the volume dataset
  vec3 dataPos = texture_coords.xyz;

  // Getting the ray marching direction:
  // get the object space position by subracting 0.5 from the
  // 3D texture coordinates. Then subtraact it from camera position
  // and normalize to  get the ray marching direction
  vec3 geomDir = normalize(vertex_pos.xyz - camPos);

  // Multiply the raymarching direction with the step size to get the
  //sub-step size we need to take at each raymarching step
  vec3 dirStep = geomDir * step_size;

  // Flag to indicate if the raymarch loop should terminate
  bool stop = false;

  //for all samples along the ray
  for (int i = 0; i < MAX_SAMPLES; i++) {
    // Advance ray by dirstep
    dataPos = dataPos + dirStep;

    //The two constants texMin and texMax have a value of vec3(-1,-1,-1)
    //and vec3(1,1,1) respectively. To determine if the data value is
    //outside the volume data, we use the sign function. The sign function
    //return -1 if the value is less than 0, 0 if the value is equal to 0
    //and 1 if value is greater than 0. Hence, the sign function for the
    //calculation (sign(dataPos-texMin) and sign (texMax-dataPos)) will
    //give us vec3(1,1,1) at the possible minimum and maximum position.
    //When we do a dot product between two vec3(1,1,1) we get the answer 3.
    //So to be within the dataset limits, the dot product will return a
    //value less than 3. If it is greater than 3, we are already out of
    //the volume dataset
    stop = dot(sign(dataPos-texMin),sign(texMax-dataPos)) < 3.0;

    //if the stopping condition is true we brek out of the ray marching loop
    if (stop)
      break;

    // Data fetching from the red channel of volume texture
    float scalar = texture(volume, dataPos).r;
    vec4 src = texture(transfer_func, scalar);
//    vec4 src = vec4(scalar);

    // Reduce the alpha to have a more transparent result
    //src.a *= .05f;

    //Opacity calculation using compositing:
    //here we use front to back compositing scheme whereby the current sample
    //value is multiplied to the currently accumulated alpha and then this product
    //is subtracted from the sample value to get the alpha from the previous steps.
    //Next, this alpha is multiplied with the current sample colour and accumulated
    //to the composited colour. The alpha value from the previous steps is then
    //accumulated to the composited colour alpha.
    //     float prev_alpha = sample - (sample * dst.a);
    //     dst.rgb = prev_alpha * vec3(sample) + dst.rgb;
    //     dst.a += prev_alpha;
    src.rgb *= src.a;
    dst = (1.0f - dst.a) * src + dst;

    //Early ra`y termination
    //if the currently composited colour alpha is already fully saturated
    //we terminated the loop
    if(dst.a > 0.99)
      break;
  }

  //dst = vec4(geomDir, 1.0);
}
