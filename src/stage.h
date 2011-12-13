#include <glib.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_opengl.h>

int check_gl_error (void);

GLuint depth_pass_program (void);

struct DepthData
{
  GLuint fbo;
  GLuint ab;
  GLuint dpp;

  GLuint _cat0_loc;

  gint _dtest;
  gint _dfunc;
  gint _dclear;
  gint _dmask;
  gint _cmask[4];
};

/* GLuint depth_pass_program (void); */
void depth_bind (struct DepthData *ds);
void depth_unbind (struct DepthData *ds);

struct MaterialFbo
{
  GLuint tdep;
  
  GLuint _fbo;
  GLuint _tnor, _tdiff, _tspec;
};

struct MaterialData
{
  GLuint fbo;
  GLuint ctex;
  GLuint dtex;
  GLuint mpp;

  GLuint vb;
  GLuint nb;
  GLuint tb;

  GLuint _cat0_loc;
  GLuint _cat1_nor;
  GLuint _cat2_tex;
  GLuint _tex0;
  GLuint _diffuse, _specular;
  
  /* GLuint vertb; */
  /* GLuint normb; */
  /* GLuint textb; */
};

void material_create_fbo (struct MaterialFbo *ms);
GLuint material_pass_program (void);

void material_bind (struct MaterialData *ms);
void material_unbind (struct MaterialData *ms);
