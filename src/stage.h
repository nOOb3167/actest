#include <glib.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_opengl.h>

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

void depth_bind (struct DepthData *ds);
void depth_unbind (struct DepthData *ds);
