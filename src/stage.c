#include <stdio.h>
#include <src/error.h>
#include <src/stage.h>
#include <src/vshd_src.h>

int
check_gl_error (void)
{
  GLenum err;
  err = glGetError ();
  if (GL_NO_ERROR != err) printf ("GLERROR: %x\n", err);
  g_xassert (GL_NO_ERROR == err);
}

GLuint
depth_pass_program (void)
{
  GLint status;
  GLuint vshd;
  GLuint prog;

  vshd = glCreateShader (GL_VERTEX_SHADER);
  glShaderSource (vshd, 1, vshd_depth_src, NULL);
  glCompileShader (vshd);
  
  glGetShaderiv (vshd, GL_COMPILE_STATUS, &status);
  g_xassert (GL_TRUE == status);

  check_gl_error ();

  prog = glCreateProgram ();
  glAttachShader (prog, vshd);
  glLinkProgram (prog);
  
  glGetProgramiv (prog, GL_LINK_STATUS, &status);
  g_xassert (GL_TRUE == status);

  check_gl_error ();

  return prog;
}

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

GLuint
material_pass_program (void)
{
  GLint status;
  GLuint vshd, fshd;
  GLuint prog;

  vshd = glCreateShader (GL_VERTEX_SHADER);
  glShaderSource (vshd, 1, vshd_material_src, NULL);
  glCompileShader (vshd);
  
  glGetShaderiv (vshd, GL_COMPILE_STATUS, &status);
  g_xassert (GL_TRUE == status);

  check_gl_error ();

  fshd = glCreateShader (GL_FRAGMENT_SHADER);
  glShaderSource (fshd, 1, fshd_material_src, NULL);
  glCompileShader (fshd);
  
  glGetShaderiv (fshd, GL_COMPILE_STATUS, &status);
  g_xassert (GL_TRUE == status);

  check_gl_error ();

  prog = glCreateProgram ();
  glAttachShader (prog, vshd);
  glAttachShader (prog, fshd);
  glLinkProgram (prog);
  
  glGetProgramiv (prog, GL_LINK_STATUS, &status);
  //g_xassert (GL_TRUE == status);
  if (GL_TRUE != status)
    {
      GLsizei len;
      GLchar infolog[2048];
      glGetProgramInfoLog (prog, sizeof (infolog), &len, infolog);
      printf ("DUMPING PROGRAM INFO LOG\n%s", infolog);
      g_xassert (0);
    }

  check_gl_error ();

  return prog;
}

void
material_create_fbo (struct MaterialFbo *ms)
{
  g_xassert (ms->tdep);

  glGenTextures (1, &ms->_tnor);
  glActiveTexture (GL_TEXTURE0);
  glBindTexture (GL_TEXTURE_2D, ms->_tnor);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8,
		640, 480, 0,
		GL_RGBA, GL_UNSIGNED_BYTE,
		NULL);

  glGenTextures (1, &ms->_tdiff);
  glActiveTexture (GL_TEXTURE0);
  glBindTexture (GL_TEXTURE_2D, ms->_tdiff);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8,
		640, 480, 0,
		GL_RGBA, GL_UNSIGNED_BYTE,
		NULL);

  glGenTextures (1, &ms->_tspec);
  glActiveTexture (GL_TEXTURE0);
  glBindTexture (GL_TEXTURE_2D, ms->_tspec);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8,
		640, 480, 0,
		GL_RGBA, GL_UNSIGNED_BYTE,
		NULL);

  glGenFramebuffersEXT (1, &ms->_fbo);
  glBindFramebufferEXT (GL_FRAMEBUFFER, ms->_fbo);
  glFramebufferTexture2DEXT (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			     GL_TEXTURE_2D, ms->_tnor, 0);
  glFramebufferTexture2DEXT (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
			     GL_TEXTURE_2D, ms->_tdiff, 0);
  glFramebufferTexture2DEXT (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2,
			     GL_TEXTURE_2D, ms->_tspec, 0);
  glFramebufferTexture2DEXT (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
			     GL_TEXTURE_2D, ms->tdep, 0);
  
  g_xassert (GL_FRAMEBUFFER_COMPLETE ==
	   glCheckFramebufferStatusEXT (GL_FRAMEBUFFER));
}

void
material_bind (struct MaterialData *ms)
{
  ms->_cat0_loc = glGetAttribLocation (ms->mpp, "cat0");
  ms->_cat1_nor = glGetAttribLocation (ms->mpp, "cat1");
  ms->_cat2_tex = glGetAttribLocation (ms->mpp, "cat2");
  ms->_tex0 = glGetUniformLocation (ms->mpp, "tex0");
  ms->_diffuse = glGetUniformLocation (ms->mpp, "diffuse");
  ms->_specular = glGetUniformLocation (ms->mpp, "specular");

  g_xassert (-1 != ms->_cat0_loc);
  g_xassert (-1 != ms->_cat1_nor);
  g_xassert (-1 != ms->_cat2_tex);
  g_xassert (-1 != ms->_tex0);
  g_xassert (-1 != ms->_diffuse &&
             -1 != ms->_specular);
  
  glEnableVertexAttribArray (ms->_cat0_loc);
  glEnableVertexAttribArray (ms->_cat1_nor);
  glEnableVertexAttribArray (ms->_cat2_tex);

  glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, ms->fbo);

  glPushAttrib (GL_VIEWPORT_BIT | GL_TRANSFORM_BIT);
  glViewport (0, 0, 640, 480);
}

void
material_unbind (struct MaterialData *ms)
{
  glPopAttrib ();
  glDisableVertexAttribArray (ms->_cat0_loc);
  glDisableVertexAttribArray (ms->_cat1_nor);
  glDisableVertexAttribArray (ms->_cat2_tex);
  glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
}
