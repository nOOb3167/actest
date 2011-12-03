#include <stdlib.h>
#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_opengl.h>
#include <allegro5/allegro_image.h>
#include <assert.h>

#include <assimp.h>
#include <aiScene.h>
#include <aiPostProcess.h>
#include <src/error.h>
/* #include <src/mai-model.h> */
#include <src/stage.h>

#define xassert(exp) do { ((exp)?0:++(*((char *)0x00000010))); } while (0)

#include <src/vshd_src.h>

ALLEGRO_BITMAP *blender_bmp;
GLuint blender_tex;

/**
 * Immediate mode
 *
 * Depends on Matrix state
 * Depends on Viewport
 * Depends on DrawBuffer? (Certainly can't do COLOR_ATTACHMENT with FBO 0)
 * 
 * Overwrites FRAMEBUFFER_BINDING
 * Overwrites UseProgram
 * Overwrites Texture state
 * Overwrites Array state
 */
void
debug_draw_tex_quad (GLuint tex, float x, float y, float w, float h)
{
  check_gl_error ();

  glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);

  glActiveTexture (GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex);
  glEnable (GL_TEXTURE_2D);
  glUseProgram (0);
  glDisableVertexAttribArray (0);

  glBegin(GL_QUADS);
  glTexCoord2f(0, 0); glVertex2f(x  , y  );
  glTexCoord2f(1, 0); glVertex2f(x+w, y  );
  glTexCoord2f(1, 1); glVertex2f(x+w, y+h);
  glTexCoord2f(0, 1); glVertex2f(x  , y+h);
  glEnd();

  check_gl_error ();
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

void
mesh_arrays_extract (struct aiMesh *me,
                     GLfloat **v,
                     GLfloat **n,
                     GLfloat **t,
                     gint *len_out)
{
  gint req;
  GLfloat *abbuf;
  
  /* Ensure triangles only */
  g_xassert (aiPrimitiveType_TRIANGLE == me->mPrimitiveTypes);

  g_xassert (me->mNumVertices &&
             /**
              * Well looks like noone thought to provide that in C API
              * // me->mNumNormals &&
              */
             2 == me->mNumUVComponents[0]);

  /* Calculate the required size.
     A triangle face has 3 vertices, of 3 float components each.
     A float takes sizeof bmu. */
  req = me->mNumFaces * 3 * 3 * sizeof (*abbuf);

  *v = (GLfloat *)g_malloc (req);
  *n = (GLfloat *)g_malloc (req);
  *t = (GLfloat *)g_malloc (req);
  xassert (v && n && t);

  for (int i = 0; i < me->mNumFaces; ++i)
    {
      struct aiFace *fa;

      fa = &me->mFaces[i];
      g_xassert (3 == fa->mNumIndices);

      for (int j = 0; j < 3; ++j)
        {
          /**
           * One triangle is 3 times 3 floats
           * f,f,f
           * f,f,f
           * f,f,f
           * One pass from this loop loads one f,f,f group.
           * Need to advance by 3 each pass, to a total of 9.
           *
           * Tex coordinates load as f,f,f at the moment even though
           * mNumUVComponents[0] might be for example 2.
           * Code wise this is not a problem as the mTextureCoords[0],
           * is an array of aiVector3D, has 3 components.
           * (With 2 UV, the third will just end up bogus)
           */
          struct aiVector3D *vt;
          struct aiVector3D *nr;
          struct aiVector3D *tx;

          vt = &me->mVertices[fa->mIndices[j]];
          nr = &me->mNormals[fa->mIndices[j]];
          tx = &me->mTextureCoords[0][fa->mIndices[j]];

          (*v)[(3 * 3 * i) + (3 * j + 0)] = vt->x;
          (*v)[(3 * 3 * i) + (3 * j + 1)] = vt->y;
          (*v)[(3 * 3 * i) + (3 * j + 2)] = vt->z;
          (*n)[(3 * 3 * i) + (3 * j + 0)] = nr->x;
          (*n)[(3 * 3 * i) + (3 * j + 1)] = nr->y;
          (*n)[(3 * 3 * i) + (3 * j + 2)] = nr->z;
          (*t)[(3 * 3 * i) + (3 * j + 0)] = tx->x;
          (*t)[(3 * 3 * i) + (3 * j + 1)] = tx->y;
          (*t)[(3 * 3 * i) + (3 * j + 2)] = tx->z;
        }      
    }

  *len_out = req;
}

GLuint
mesh_buffers_extract (struct aiMesh *me,
                      GLuint *vb_out,
                      GLuint *nb_out,
                      GLuint *tb_out)
{
  gint req;
  GLuint vb, nb, tb;
  GLfloat *v, *n, *t;
  
  mesh_arrays_extract (me, &v, &n, &t, &req);
  
  glGenBuffers (1, &vb);
  glBindBuffer (GL_ARRAY_BUFFER, vb);
  glBufferData (GL_ARRAY_BUFFER,
                req, v,
                GL_STREAM_DRAW);

  glGenBuffers (1, &nb);
  glBindBuffer (GL_ARRAY_BUFFER, nb);
  glBufferData (GL_ARRAY_BUFFER,
                req, n,
                GL_STREAM_DRAW);

  glGenBuffers (1, &tb);
  glBindBuffer (GL_ARRAY_BUFFER, tb);
  glBufferData (GL_ARRAY_BUFFER,
                req, t,
                GL_STREAM_DRAW);

  glBindBuffer (GL_ARRAY_BUFFER, 0);

  *vb_out = vb;
  *nb_out = nb;
  *tb_out = tb;
}

void
derp (void)
{
  const struct aiScene *csc;
  struct aiScene *sc;
  struct aiMesh *me;
  GLuint vb, nb, tb;
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

  mesh_buffers_extract (me, &vb, &nb, &tb);

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
  ds.ab = vb;
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

  debug_draw_tex_quad (ctex, 100, 0, 100, 100);
  debug_draw_tex_quad (dtex, 100, 150, 100, 100);

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

  /**
   * Warning:
   * Remember to unbind the depth texture (dtex) from depth_stage fbo,
   * Rebind it as texture for rendering. (Don't need to unbind?)
   */
  struct MaterialData ms;
  ms.fbo = fbo;
  ms.ctex = ctex; /* Load the mesh texture somehow, dummy for now */
  ms.dtex = dtex;
  ms.mpp = material_pass_program ();
  ms.vb = vb;
  ms.nb = nb;
  ms.tb = tb;

  material_bind (&ms);

  {
    glMatrixMode (GL_PROJECTION);
    glPushMatrix ();
    glLoadIdentity ();
    glOrtho (-2.5, 2.5, -2.5, 2.5, -5, 5);
    //gluPerspective (45.0f, 1.0f, 0.1f, 10.1f);


    GLenum material_draw_buffers[] = {GL_COLOR_ATTACHMENT0,
                                      GL_COLOR_ATTACHMENT1,
                                      GL_COLOR_ATTACHMENT2};
    glDrawBuffers (3, material_draw_buffers);

    glBindBuffer (GL_ARRAY_BUFFER, ms.vb);
    glVertexAttribPointer (ms._cat0_loc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer (GL_ARRAY_BUFFER, ms.nb);
    glVertexAttribPointer (ms._cat1_nor, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer (GL_ARRAY_BUFFER, ms.tb);
    glVertexAttribPointer (ms._cat2_tex, 3, GL_FLOAT, GL_FALSE, 0, 0);

    /* Program needs to be active to load Uniforms */
    glUseProgram (ms.mpp);

    /**
     * Texture unit 0,
     * make sure diffuse / color texture is glBindTexture'd
     * while glActiveTexture is zero.
     */
    glUniform1i (ms._tex0, 0);

    /* Also go load diffuse, specular out of the aiColor4Ds */
    
    glActiveTexture (GL_TEXTURE0);
    glEnable (GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, ms.ctex);

    glDrawArrays (GL_TRIANGLES, 0, me->mNumFaces * 3);

    glUseProgram (0);

    glMatrixMode (GL_PROJECTION);
    glPopMatrix ();
  }

  material_unbind (&ms);

  debug_draw_tex_quad (ctex, 200, 0, 100, 100);
}

int
main (int argc, char **argv)
{
  g_type_init ();
  
  al_init ();
  al_init_image_addon ();

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

  printf ("OPENGL %x\n", al_get_opengl_version ());


  blender_bmp = al_load_bitmap ("../data/n1img0.bmp");
  g_xassert (640 == al_get_bitmap_width (blender_bmp) &&
             480 == al_get_bitmap_height (blender_bmp));
  blender_tex = al_get_opengl_texture (blender_bmp);
  g_xassert (blender_tex);


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
