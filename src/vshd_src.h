/**
 * ERR LET ME GET THIS RIGHT:
 * ARE YOU REALLY TELLING ME THE SHADER COMPILER
 * CAN USE STATIC ANALYSIS AND OPTIMISE ATTRIBUTE VARIABLES OUT
 * SO glGetAttribLocation FAILS?
 * THIS IS INSANE, RETARDED, THEN INSANE AGAIN.
 *
 * Also don't use custom attributes and fixed pipeline together
 * (If one of a vertex/fragment shader is not present, that counts as ffp!)
 *
 * In Blender Collada export plugin do not check "Use UV Image Mats",
 * doing so seems not to produce material related data.
 */

static const GLchar *vshd_depth_src[] = {"\
attribute vec4 cat0;\
\
void main(void)\
{\
  gl_Position = gl_ModelViewProjectionMatrix * cat0;\
}\
"};

static const GLchar *vshd_material_src[] = {"\
attribute vec4 cat0;\
attribute vec4 cat1;\
attribute vec4 cat2;\
uniform sampler2D tex0;\
\
uniform vec4 diffuse;\
uniform vec4 specular;\
\
varying vec4 nor;\
varying vec4 dif;\
varying vec4 spe;\
varying vec2 uv;\
\
void main(void)\
{\
  nor = cat1;\
  spe = specular;\
  dif = diffuse;\
  uv = vec2(cat2.xy);\
  gl_Position = gl_ModelViewProjectionMatrix * cat0;\
}\
"};

static const GLchar *fshd_material_src[] = {"\
attribute vec4 cat0;\
attribute vec4 cat1;\
attribute vec4 cat2;\
uniform sampler2D tex0;\
\
uniform vec4 diffuse;\
uniform vec4 specular;\
\
varying vec4 nor;\
varying vec4 dif;\
varying vec4 spe;\
varying vec2 uv;\
\
void main(void)\
{\
  vec4 color;\
\
  color = texture2D(tex0, uv);\
\
  gl_FragData[0] = vec4(nor.xyz, 0);\
  gl_FragData[1] = vec4(color*dif);\
  gl_FragData[2] = vec4(spe);\
}\
"};

static const GLchar *vshd_lighting_ambient_src[] = {"\
attribute vec4 cat0;\
void main(void)\
{\
  gl_Position = gl_ModelViewProjectionMatrix * cat0;\
}\
"};

static const GLchar *fshd_lighting_ambient_src[] = {"\
attribute vec4 cat0;\
void main(void)\
{\
  gl_FragData[0] = vec4(1.0, 0.0, 0.0, 1.0);\
}\
"};

static const GLchar *vshd_src[] = {"\
attribute vec4 cat0;\
\
void main(void)\
{\
  gl_Position = gl_ModelViewProjectionMatrix * cat0;\
}\
"};

static const GLchar *fshd_src[] = {"\
void main()\
{\
  gl_FragData[0] = vec4(1.0, 0.0, 0.0, 1.0);\
  gl_FragData[1] = vec4(1.0, 1.0, 0.0, 1.0);\
}\
"};
