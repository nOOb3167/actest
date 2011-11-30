a#include <stdlib.h>
#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_opengl.h>
#include <assert.h>

#include <assimp.h>
#include <aiScene.h>
#include <aiPostProcess.h>
#include <src/error.h>
/* #include <src/mai-model.h> */
#include <src/stage.h>

#define xassert(exp) do { ((exp)?0:++(*((char *)0x00000010))); } while (0)

#include <src/vshd_src.h>

int check_gl_error (void)
{
  GLenum err;
  err = glGetError ();
  if (GL_NO_ERROR != err) printf ("GLERROR: %x\n", err);
  xassert (GL_NO_ERROR == err);
}

int
make_fbo (void)
{
  check_gl_error ();

  GLenum err;
  
  GLuint tex;
  GLuint tex2;

  glGenTextures (1, &tex);
  glActiveTexture (GL_TEXTURE0);
  glBindTexture (GL_TEXTURE_2D, tex);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA,
		640, 480, 0,
		GL_RGBA, GL_UNSIGNED_BYTE,
		NULL);

  glGenTextures (1, &tex2);
  glActiveTexture (GL_TEXTURE0);
  glBindTexture (GL_TEXTURE_2D, tex2);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA,
		640, 480, 0,
		GL_RGBA, GL_UNSIGNED_BYTE,
		NULL);

  GLuint fbo;

  glGenFramebuffersEXT (1, &fbo);
  glBindFramebufferEXT (GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2DEXT (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			     GL_TEXTURE_2D, tex, 0);

  glFramebufferTexture2DEXT (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
			     GL_TEXTURE_2D, tex2, 0);

  xassert (GL_FRAMEBUFFER_COMPLETE ==
	   glCheckFramebufferStatusEXT (GL_FRAMEBUFFER));

  check_gl_error ();

  GLint status;

  GLuint vshd;
  GLuint fshd;

  vshd = glCreateShader (GL_VERTEX_SHADER);
  glShaderSource (vshd, 1, vshd_src, NULL);
  glCompileShader (vshd);
  
  glGetShaderiv (vshd, GL_COMPILE_STATUS, &status);
  xassert (GL_TRUE == status);

  fshd = glCreateShader (GL_FRAGMENT_SHADER);
  glShaderSource (fshd, 1, fshd_src, NULL);
  glCompileShader (fshd);
  
  glGetShaderiv (fshd, GL_COMPILE_STATUS, &status);
  xassert (GL_TRUE == status);

  GLuint prog;

  check_gl_error ();

  prog = glCreateProgram ();
  glAttachShader (prog, vshd);
  glAttachShader (prog, fshd);  
  glLinkProgram (prog);
  
  glGetProgramiv (prog, GL_LINK_STATUS, &status);
  xassert (GL_TRUE == status);

  check_gl_error ();

  GLint cat0_loc;

  cat0_loc = glGetAttribLocation (prog, "cat0");
  xassert (-1 != cat0_loc);

  check_gl_error ();

  GLuint ab;

  GLubyte *abbuf;
  GLfloat *abbuf_f;
  abbuf = malloc (4*4 * 3);
  abbuf_f = (GLfloat *)abbuf;
  xassert (abbuf);

  abbuf_f[0] = 0.0f;
  abbuf_f[1] = 0.0f;
  abbuf_f[2] = 0.0f;
  abbuf_f[3] = 1.0f;
  abbuf_f[4] = 100.0f;
  abbuf_f[5] = 0.0f;
  abbuf_f[6] = 0.0f;
  abbuf_f[7] = 1.0f;
  abbuf_f[8] = 50.0f;
  abbuf_f[9] = 100.0f;
  abbuf_f[10] = 0.0f;
  abbuf_f[11] = 1.0f;

  glGenBuffers (1, &ab);
  glBindBuffer (GL_ARRAY_BUFFER, ab);
  glBufferData (GL_ARRAY_BUFFER,
		4*4 * 3, abbuf,
		GL_STREAM_DRAW);

  /* glVertexAttribPointer acts on the buffer object currently
     bound to ARRAY_BUFFER_BINDING, the buffer object
     becomes 'baked' into VERTEX_ATTRIB_ARRAY_BUFFER_BINDING
     for the specified index (cat0_loc in this case) */
  /* For indices, also use ELEMENT_ARRAY_BUFFER, with DrawElements (2.9.7) */
  glVertexAttribPointer (cat0_loc, 4, GL_FLOAT, GL_FALSE, 0, 0);

  /* Had a bug: Calling with 0 instead of cat0_loc */
  glEnableVertexAttribArray (cat0_loc);

  check_gl_error ();

  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();

  glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, fbo);

  {
    glPushAttrib (GL_VIEWPORT_BIT | GL_TRANSFORM_BIT);
    glViewport (0, 0, 640, 480);
    glClearColor (0, 1, 0, 1);
    glClear (GL_COLOR_BUFFER_BIT);

    {
      glMatrixMode (GL_PROJECTION);
      glPushMatrix ();
      glLoadIdentity ();
      glOrtho (0, 256, 256, 0, -1, 1);
  
      GLenum draw_buffers[] = {GL_COLOR_ATTACHMENT0,
			       GL_COLOR_ATTACHMENT1};
      glDrawBuffers (2, draw_buffers);
      glUseProgram (prog);

      glDrawArrays (GL_TRIANGLES, 0, 3);

      glMatrixMode (GL_PROJECTION);
      glPopMatrix ();
    }

    glPopAttrib ();
  }

  glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);

  check_gl_error ();

  glClearColor (0, 0, 1, 1);
  glClear (GL_COLOR_BUFFER_BIT);

  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();

  glEnable (GL_TEXTURE_2D);
  glUseProgram (0);

  glBindTexture(GL_TEXTURE_2D, tex);
  glBegin(GL_QUADS);
  glTexCoord2f(0, 0); glVertex2f(0, 0);
  glTexCoord2f(1, 0); glVertex2f(100, 0);
  glTexCoord2f(1, 1); glVertex2f(100, 100);
  glTexCoord2f(0, 1); glVertex2f(0, 100);
  glEnd();

  glBindTexture(GL_TEXTURE_2D, tex2);
  glBegin(GL_QUADS);
  glTexCoord2f(0, 0); glVertex2f(0, 150);
  glTexCoord2f(1, 0); glVertex2f(100, 150);
  glTexCoord2f(1, 1); glVertex2f(100, 250);
  glTexCoord2f(0, 1); glVertex2f(0, 250);
  glEnd();
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
  xassert (GL_TRUE == status);

  check_gl_error ();

  prog = glCreateProgram ();
  glAttachShader (prog, vshd);
  glLinkProgram (prog);
  
  glGetProgramiv (prog, GL_LINK_STATUS, &status);
  xassert (GL_TRUE == status);

  check_gl_error ();

  return prog;
}

GLfloat *
mesh_to_float (struct aiMesh *me, gint *len_out)
{
  gint req;
  GLfloat *abbuf;
  
  /* Ensure triangles only */
  g_xassert (aiPrimitiveType_TRIANGLE == me->mPrimitiveTypes);

  /* Calculate the required size.
     A triangle face has 3 vertices, of 3 float components each.
     A float takes sizeof bmu. */
  req = me->mNumFaces * 3 * 3 * sizeof (*abbuf);

  abbuf = (GLfloat *)g_malloc (req);
  xassert (abbuf);

  for (int i = 0; i < me->mNumFaces; ++i)
    {
      struct aiFace *f;
      struct aiVector3D *v;

      f = &me->mFaces[i];
      g_xassert (3 == f->mNumIndices);

      v = &me->mVertices[f->mIndices[0]];
      abbuf[(3 * 3 * i) + 0] = v->x;
      abbuf[(3 * 3 * i) + 1] = v->y;
      abbuf[(3 * 3 * i) + 2] = v->z;

      v = &me->mVertices[f->mIndices[1]];
      abbuf[(3 * 3 * i) + 3] = v->x;
      abbuf[(3 * 3 * i) + 4] = v->y;
      abbuf[(3 * 3 * i) + 5] = v->z;

      v = &me->mVertices[f->mIndices[2]];
      abbuf[(3 * 3 * i) + 6] = v->x;
      abbuf[(3 * 3 * i) + 7] = v->y;
      abbuf[(3 * 3 * i) + 8] = v->z;
    }

  *len_out = req;

  return abbuf;
}

GLuint
mesh_to_buffer (struct aiMesh *me)
{
  gint req;
  GLuint ab;
  GLfloat *abbuf;
  
  abbuf = mesh_to_float (me, &req);
  g_xassert (abbuf);
  
  glGenBuffers (1, &ab);
  glBindBuffer (GL_ARRAY_BUFFER, ab);
  glBufferData (GL_ARRAY_BUFFER,
                req, abbuf,
                GL_STREAM_DRAW);
  glBindBuffer (GL_ARRAY_BUFFER, 0);

  return ab;
}

void
derp (void)
{
  const struct aiScene *csc;
  struct aiScene *sc;
  struct aiMesh *me;
  GLuint ab;
  GLuint ctex;
  GLuint dtex;
  GLuint fbo;
  GLuint dpp;
  GLint dpp_cat0_loc;

  csc = aiImportFile ("../data/n1.dae",
                      aiProcess_Triangulate | aiProcess_PreTransformVertices);
  sc = (struct aiScene *)csc;
  
  g_xassert (sc);
  g_xassert (sc->mNumMeshes >= 1);
  g_xassert (sc->mNumMaterials >= 1);

  me = sc->mMeshes[0];

  g_xassert (aiPrimitiveType_TRIANGLE == me->mPrimitiveTypes);

  ab = mesh_to_buffer (me);

  /* Create two textures, a color and a depth. */

  glGenTextures (1, &ctex);
  glActiveTexture (GL_TEXTURE0);
  glBindTexture (GL_TEXTURE_2D, ctex);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8,
		640, 480, 0,
		GL_RGBA, GL_UNSIGNED_BYTE,
		NULL);

  glGenTextures (1, &dtex);
  glActiveTexture (GL_TEXTURE0);
  glBindTexture (GL_TEXTURE_2D, dtex);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24_ARB,
		640, 480, 0,
		GL_DEPTH_COMPONENT, GL_UNSIGNED_INT,
		NULL);

  check_gl_error ();

  /* Create a FBO and attach the newly created textures */

  glGenFramebuffersEXT (1, &fbo);
  glBindFramebufferEXT (GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2DEXT (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			     GL_TEXTURE_2D, ctex, 0);
  glFramebufferTexture2DEXT (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
			     GL_TEXTURE_2D, dtex, 0);
  
  g_xassert (GL_FRAMEBUFFER_COMPLETE ==
	   glCheckFramebufferStatusEXT (GL_FRAMEBUFFER));

  check_gl_error ();

  /* Create the depth pass program. (Vertex shader only, do nothing) */

  dpp = depth_pass_program ();

  check_gl_error ();

  struct DepthData ds;
  ds.fbo = fbo;
  ds.ab = ab;
  ds.dpp = dpp;

  depth_bind (&ds);

  {
    glMatrixMode (GL_PROJECTION);
    glPushMatrix ();
    glLoadIdentity ();
    glOrtho (-2.5, 2.5, -2.5, 2.5, -5, 5);
    //gluPerspective (45.0f, 1.0f, 0.1f, 10.1f);


    glDrawBuffer (GL_COLOR_ATTACHMENT0);
    glClear (GL_DEPTH_BUFFER_BIT);

    glBindBuffer (GL_ARRAY_BUFFER, ds.ab);
    glVertexAttribPointer (ds._cat0_loc, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glUseProgram (ds.dpp);
    glDrawArrays (GL_TRIANGLES, 0, me->mNumFaces * 3);
    glUseProgram (0);

    glMatrixMode (GL_PROJECTION);
    glPopMatrix ();
  }

  depth_unbind (&ds);

  check_gl_error ();

  /* Draw depth texture */
  glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);

  check_gl_error ();

  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();

  glEnable (GL_TEXTURE_2D);
  glUseProgram (0);

  glBindTexture(GL_TEXTURE_2D, ctex);
  glBegin(GL_QUADS);
  glTexCoord2f(0, 0); glVertex2f(100, 0);
  glTexCoord2f(1, 0); glVertex2f(200, 0);
  glTexCoord2f(1, 1); glVertex2f(200, 100);
  glTexCoord2f(0, 1); glVertex2f(100, 100);
  glEnd();

  glBindTexture(GL_TEXTURE_2D, dtex);
  glBegin(GL_QUADS);
  glTexCoord2f(0, 0); glVertex2f(100, 150);
  glTexCoord2f(1, 0); glVertex2f(200, 150);
  glTexCoord2f(1, 1); glVertex2f(200, 250);
  glTexCoord2f(0, 1); glVertex2f(100, 250);
  glEnd();

  check_gl_error ();


  /**
   * Material stuff, move elsewhere or something.
   */
  struct aiMaterial *mat;
  struct aiColor4D col_diff;
  struct aiColor4D col_spec;

  g_xassert (sc->mNumMaterials >= 1);

  mat = sc->mMaterials[me->mMaterialIndex];

  g_xassert (AI_SUCCESS ==
             aiGetMaterialColor (mat, AI_MATKEY_COLOR_DIFFUSE, &col_diff));

  g_xassert (AI_SUCCESS ==
             aiGetMaterialColor (mat, AI_MATKEY_COLOR_SPECULAR, &col_spec));
}

int
main (int argc, char **argv)
{
  g_type_init ();
  
  al_init ();

  al_set_new_display_option (ALLEGRO_RED_SIZE, 8,  ALLEGRO_REQUIRE);
  al_set_new_display_option (ALLEGRO_GREEN_SIZE, 8, ALLEGRO_REQUIRE);
  al_set_new_display_option (ALLEGRO_BLUE_SIZE, 8, ALLEGRO_REQUIRE);
  al_set_new_display_option (ALLEGRO_ALPHA_SIZE, 8, ALLEGRO_REQUIRE);
  /* Nouveau = good */
  /* al_set_new_display_option (ALLEGRO_AUX_BUFFERS, 4, ALLEGRO_REQUIRE); */

  al_set_new_display_flags (ALLEGRO_WINDOWED | ALLEGRO_OPENGL);

  ALLEGRO_DISPLAY *disp;
  disp = al_create_display (640, 480);
  xassert (disp);

  g_xassert (al_get_opengl_extension_list ()->ALLEGRO_GL_ARB_depth_texture);
  g_xassert (al_get_opengl_extension_list ()->ALLEGRO_GL_ARB_framebuffer_object);

  al_set_target_backbuffer (disp);

  ALLEGRO_BITMAP *bmp;
  bmp = al_create_bitmap (640, 480);
  xassert (bmp);

  printf ("OPENGL %x\n", al_get_opengl_version ());

  GLint glvar;
  /* Query GL_MAX_DRAW_BUFFERS, GL_MAX_COLOR_ATTACHMENTS */
  glGetIntegerv (GL_MAX_DRAW_BUFFERS, &glvar);
  xassert (4 <= glvar);
  glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &glvar);
  xassert (4 <= glvar);

  /* mmstuff (); */

  make_fbo ();

  derp ();

  al_flip_display ();

  al_rest (2);

  return EXIT_SUCCESS;
}
