#include <src/stage.h>

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

  glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, ds->fbo);

  glBindBuffer (GL_ARRAY_BUFFER, ds->ab);
  glEnableVertexAttribArray (ds->cat0_loc);
  glVertexAttribPointer (ds->cat0_loc, 3, GL_FLOAT, GL_FALSE, 0, 0);

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
  glDisableVertexAttribArray (ds->cat0_loc);
  glBindBuffer (GL_ARRAY_BUFFER, 0);
  glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
}
