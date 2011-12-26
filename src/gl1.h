#ifndef GL1_H_
#define GL1_H_

#include <assimp.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_opengl.h>

#define xassert(exp) do { ((exp)?0:++(*((char *)0x00000010))); } while (0)

#define RESTORE_MODE_SAVE 1
#define RESTORE_MODE_RESTORE 2
#define RESTORE_SUB_ALLEGRO 0
#define RESTORE_SUB_CUSTOM 1

void debug_draw_tex_quad (GLuint tex, float x, float y, float w, float h);

void mesh_arrays_extract (struct aiMesh *me,
                          GLfloat **v,
                          GLfloat **n,
                          GLfloat **t,
                          gint *len_out);

GLuint mesh_buffers_extract (struct aiMesh *me,
                             GLuint *vb_out,
                             GLuint *nb_out,
                             GLuint *tb_out);

void allegro_ffp_restore (int mode, int sub);

#endif /* GL1_H_ */
