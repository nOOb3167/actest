#include <stdlib.h>
#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_opengl.h>
#include <allegro5/allegro_image.h>
#include <assert.h>
#include <GL/glu.h>

#include <assimp.h>
#include <aiScene.h>
#include <aiPostProcess.h>
#include <src/error.h>
/* #include <src/mai-model.h> */
#include <src/stage.h>
#include <src/gl1.h>

#include <src/vshd_src.h>

ALLEGRO_BITMAP *blender_bmp;
GLuint blender_tex;

/**
 * Immediate mode
 *
 * While it:
 *   Depends on Matrix state
 *   Depends on Viewport
 *   The code does attempt to save those with a call to allegro_ffp_restore.
 *   allegro_ffp_restore RESTORE_SUB_ALLEGRO must have been initialized.
 *
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
  allegro_ffp_restore (RESTORE_MODE_SAVE, RESTORE_SUB_CUSTOM);
  allegro_ffp_restore (RESTORE_MODE_RESTORE, RESTORE_SUB_ALLEGRO);

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

  allegro_ffp_restore (RESTORE_MODE_RESTORE, RESTORE_SUB_CUSTOM);

  check_gl_error ();
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
allegro_ffp_restore (int mode, int sub)
{
  static float pr[2][16] = {0},
               mv[2][16] = {0};
  static float vp[2][4] = {0};

  g_xassert (RESTORE_SUB_ALLEGRO == sub ||
             RESTORE_SUB_CUSTOM == sub);

  if (RESTORE_MODE_SAVE == mode)
    {
      int prd, mvd;

      glGetIntegerv (GL_MODELVIEW_STACK_DEPTH, &mvd);
      glGetIntegerv (GL_PROJECTION_STACK_DEPTH, &prd);
      g_xassert (1 == prd && 1 == mvd);
        
      glGetFloatv (GL_VIEWPORT, vp[sub]);
      glGetFloatv (GL_MODELVIEW_MATRIX, mv[sub]);
      glGetFloatv (GL_PROJECTION_MATRIX, pr[sub]);
    }
  else if (RESTORE_MODE_RESTORE == mode)
    {
      int prd, mvd;

      glGetIntegerv (GL_MODELVIEW_STACK_DEPTH, &mvd);
      glGetIntegerv (GL_PROJECTION_STACK_DEPTH, &prd);
      g_xassert (1 == prd && 1 == mvd);

      glViewport (vp[sub][0], vp[sub][1], vp[sub][2], vp[sub][3]);

      {
        int mm;

        glGetIntegerv (GL_MATRIX_MODE, &mm);
        glMatrixMode (GL_MODELVIEW);
        glLoadMatrixf (mv[sub]);
        glMatrixMode (GL_PROJECTION);
        glLoadMatrixf (pr[sub]);
        glMatrixMode (mm);
      }
    }
  else
    xassert (0);
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

  /* Save Viewport, Modelview, Projection.
   * Then set them to sane defaults.
   *
   * Empirical test indicates projection set to Ortho,
   * Modelview set to Identity, Viewport set to 0,0,w,h.
   *
   * ModelView and Viewport are fine but Projection has to go,
   * as I plan setting it myself.
   */

  allegro_ffp_restore (RESTORE_MODE_SAVE, RESTORE_SUB_ALLEGRO);

  /*
   * Do not do this...
   * While it is useful to play it safe and save & restore viewport/mst,
   * I am not actually supposed to set over allegro's stuff, right?
   * Actually:
   *    Need to use allegro's original settings when using the
   *    drawing routines targeting framebuffer 0. (eg debug_draw_tex_quad)
   * It is correct to LoadIdentity the projection stack,
   * but fix said routines to use allegro state.
   */

  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  glMatrixMode (GL_MODELVIEW);

  /* Clear the default framebuffer (Optional) */

  glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
  glClearColor (0, 0, 1, 1);
  glClear (GL_COLOR_BUFFER_BIT);

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
    //glOrtho (-2.5, 2.5, -2.5, 2.5, -5, 5);
    gluPerspective (45.0f, 1.0f, 0.1f, 3.0f);

    glMatrixMode (GL_MODELVIEW);
    glPushMatrix ();
    glLoadIdentity ();
    glTranslatef (0.0f, 0.0f, -2.0f);

    glDrawBuffer (GL_COLOR_ATTACHMENT0);
    glClear (GL_DEPTH_BUFFER_BIT);

    glBindBuffer (GL_ARRAY_BUFFER, ds.ab);
    glVertexAttribPointer (ds._cat0_loc, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glUseProgram (ds.dpp);
    glDrawArrays (GL_TRIANGLES, 0, me->mNumFaces * 3);
    glUseProgram (0);

    glMatrixMode (GL_PROJECTION);
    glPopMatrix ();

    glMatrixMode (GL_MODELVIEW);
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

  struct MaterialFbo mf = {0};
  mf.tdep = dtex;

  material_create_fbo (&mf);

  /**
   * Warning:
   * Remember to unbind the depth texture (dtex) from depth_stage fbo,
   * Rebind it as texture for rendering. (Don't need to unbind?)
   * (4.4.2) ""A single framebuffer-attachable image may be attached
   * to multiple framebuffer objects""
   * Not sure about whether unbind required.
   * (4.4.3) Warns about Feedback loops, which do apply.
   */
  struct MaterialData ms = {0};
  ms.fbo = mf._fbo;
  ms.ctex = blender_tex;
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
    gluPerspective (45.0f, 1.0f, 0.1f, 3.0f);

    glMatrixMode (GL_MODELVIEW);
    glPushMatrix ();
    glLoadIdentity ();
    glTranslatef (0.0f, 0.0f, -2.0f);

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
    glUniform4f (ms._diffuse,
                 col_diff.r, col_diff.g, col_diff.b, col_diff.a);
    glUniform4f (ms._specular,
                 col_spec.r, col_spec.g, col_spec.b, col_spec.a);
    
    glActiveTexture (GL_TEXTURE0);
    glEnable (GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, ms.ctex);

    glDrawArrays (GL_TRIANGLES, 0, me->mNumFaces * 3);

    glUseProgram (0);

    glMatrixMode (GL_PROJECTION);
    glPopMatrix ();

    glMatrixMode (GL_MODELVIEW);
    glPopMatrix ();
  }

  material_unbind (&ms);

  /**
   * I believe I don't have the right COLOR_ATTACHMENTs during material stage.
   * In any case ctex probably ends up as normals.
   * Creating a new FBO for the material stage is probably ok.
   *
   * Warning:
   * For unknown reason the model, automaticall unwrapped by Blender
   * (Select all faces -> U -> Unwrap) had its UV coordinates screwed up.
   * Re-exporting with a proper UV map somehow fixed the problem.
   *
   * Blender settings: (Texure Properties)
   * Image/Source: Single Image
   * Mapping/Coordinates: UV
   * Mapping Layer: (Empty)
   * Mapping/Projection: Flat
   */
  //debug_draw_tex_quad (ctex, 200, 0, 100, 100);
  debug_draw_tex_quad (mf._tnor, 200, 0, 100, 100);
  debug_draw_tex_quad (mf._tdiff, 200, 110, 100, 100);
  debug_draw_tex_quad (mf._tspec, 200, 220, 100, 100);

  /* Restore the saved allegro settings */

  allegro_ffp_restore (RESTORE_MODE_RESTORE, RESTORE_SUB_ALLEGRO);
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

  derp ();

  al_flip_display ();

  al_rest (2);

  return EXIT_SUCCESS;
}
