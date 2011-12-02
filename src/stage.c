#include <src/error.h>
#include <src/stage.h>

/* GLuint */
/* depth_pass_program (void) */
/* { */
/*   GLint status; */
/*   GLuint vshd; */
/*   GLuint prog; */

/*   vshd = glCreateShader (GL_VERTEX_SHADER); */
/*   glShaderSource (vshd, 1, vshd_depth_src, NULL); */
/*   glCompileShader (vshd); */
  
/*   glGetShaderiv (vshd, GL_COMPILE_STATUS, &status); */
/*   xassert (GL_TRUE == status); */

/*   check_gl_error (); */

/*   prog = glCreateProgram (); */
/*   glAttachShader (prog, vshd); */
/*   glLinkProgram (prog); */
  
/*   glGetProgramiv (prog, GL_LINK_STATUS, &status); */
/*   xassert (GL_TRUE == status); */

/*   check_gl_error (); */

/*   return prog; */
/* } */

/**
 * Garbles:
 * FBO binding
 * Arrays
 * 
 * PushAttrib on GL_VIEWPORT_BIT, GL_TRANSFORM_BIT.
 */
void
depth_bind (struct DepthData *ds)
{
  /**
   * Could use GL_DEPTH_BUFFER_BIT with glPushAttrib instead etc.
   */
  glGetIntegerv (GL_DEPTH_TEST, &ds->_dtest);
  glGetIntegerv (GL_DEPTH_FUNC, &ds->_dfunc);
  glGetIntegerv (GL_DEPTH_CLEAR_VALUE, &ds->_dclear);
  glGetIntegerv (GL_DEPTH_WRITEMASK, &ds->_dmask);
  glGetIntegerv (GL_COLOR_WRITEMASK, ds->_cmask);

  glEnable (GL_DEPTH_TEST);
  glDepthFunc (GL_LEQUAL);
  glClearDepth (1.0);
  glDepthMask (GL_TRUE);
  glColorMask (GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

  ds->_cat0_loc = glGetAttribLocation (ds->dpp, "cat0");
  g_xassert (-1 != ds->_cat0_loc);
  glEnableVertexAttribArray (ds->_cat0_loc);

  glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, ds->fbo);

  glPushAttrib (GL_VIEWPORT_BIT | GL_TRANSFORM_BIT);
  glViewport (0, 0, 640, 480);
}

void
depth_unbind (struct DepthData *ds)
{
  glDepthFunc (ds->_dfunc);
  if (ds->_dtest)
    glEnable (GL_DEPTH_TEST);
  else
    glDisable (GL_DEPTH_TEST);
  glClearDepth (ds->_dclear);
  glDepthMask (ds->_dmask);
  glColorMask (ds->_cmask[0], ds->_cmask[1], ds->_cmask[2], ds->_cmask[3]);

  glPopAttrib ();
  glDisableVertexAttribArray (ds->_cat0_loc);
  glBindBuffer (GL_ARRAY_BUFFER, 0);
  glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
}

void material_bind (struct MaterialData *ms)
{
  ms->_cat0_loc = glGetAttribLocation (ms->mpp, "cat0");
  ms->_cat1_nor = glGetAttribLocation (ms->mpp, "cat1");
  ms->_cat2_tex = glGetAttribLocation (ms->mpp, "cat2");

  g_xassert (-1 != ms->_cat0_loc);
  g_xassert (-1 != ms->_cat1_nor);
  g_xassert (-1 != ms->_cat2_tex);
  
  glEnableVertexAttribArray (ms->_cat0_loc);
  glEnableVertexAttribArray (ms->_cat1_nor);
  glEnableVertexAttribArray (ms->_cat2_tex);

  glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, ms->fbo);

  glPushAttrib (GL_VIEWPORT_BIT | GL_TRANSFORM_BIT);
  glViewport (0, 0, 640, 480);
}

void material_unbind (struct MaterialData *ms)
{
}
