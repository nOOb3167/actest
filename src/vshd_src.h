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

static const GLchar *vshd_src[] = {"\
attribute vec4 cat0;   \
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
