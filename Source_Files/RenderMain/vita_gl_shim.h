#ifndef VITA_GL_SHIM_H
#define VITA_GL_SHIM_H
#ifdef __vita__

#include <vitaGL.h>
#include <string.h>
#include <stdio.h>
#include <psp2/kernel/processmgr.h>

#define VGL_SHIM_PERF 1

#define VGL_ATTR_VERTEX   0
#define VGL_ATTR_TEX0     1
#define VGL_ATTR_TEX1     2
#define VGL_ATTR_COLOR    3
#define VGL_ATTR_NORMAL   4

typedef struct {
	GLuint program;
	GLenum client_tex;
	GLfloat tex1[4];
	GLfloat fog_color[4];
	GLfloat fog_start, fog_end, fog_density;
	GLuint alpha_base, alpha_twin;
	unsigned link_gen;
	unsigned mat_gen;
	unsigned perf_draws, perf_mat, perf_switch;
	SceUInt64 perf_us_prep, perf_us_draw, perf_last;
	GLfloat gamma;
	GLfloat clip[6][4];
	unsigned clip_mask, clip_gen;
} vgl_shim_state_t;
extern vgl_shim_state_t vgl_shim_state;
#define vgl_shim_program     vgl_shim_state.program
#define vgl_shim_client_tex  vgl_shim_state.client_tex
#define vgl_shim_tex1        vgl_shim_state.tex1
#define vgl_shim_fog_color   vgl_shim_state.fog_color
#define vgl_shim_fog_start   vgl_shim_state.fog_start
#define vgl_shim_fog_end     vgl_shim_state.fog_end
#define vgl_shim_fog_density vgl_shim_state.fog_density
#define VGL_SHIM_STATE_INIT { 0, GL_TEXTURE0, { 0, 0, 0, 1 }, { 0, 0, 0, 1 }, 0.0f, 1.0f, 1.0f, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1.0f, { { 0 } }, 0, 1 }

static inline GLuint vgl_shim_tex_attr(GLenum unit) { return unit == GL_TEXTURE0 ? VGL_ATTR_TEX0 : VGL_ATTR_TEX1; }

static inline void vgl_shim_VertexPointer(GLint size, GLenum type, GLsizei stride, const void *p)
{
	glVertexPointer(size, type, stride, p);
	glVertexAttribPointer(VGL_ATTR_VERTEX, size, type, GL_FALSE, stride, p);
}

static inline void vgl_shim_TexCoordPointer(GLint size, GLenum type, GLsizei stride, const void *p)
{
	glTexCoordPointer(size, type, stride, p);
	glVertexAttribPointer(vgl_shim_tex_attr(vgl_shim_client_tex), size, type, GL_FALSE, stride, p);
}

static inline void vgl_shim_ColorPointer(GLint size, GLenum type, GLsizei stride, const void *p)
{
	glColorPointer(size, type, stride, p);
	glVertexAttribPointer(VGL_ATTR_COLOR, size, type, type == GL_UNSIGNED_BYTE ? GL_TRUE : GL_FALSE, stride, p);
}

static inline void vgl_shim_NormalPointer(GLenum type, GLsizei stride, const void *p)
{
	glNormalPointer(type, stride, p);
	glVertexAttribPointer(VGL_ATTR_NORMAL, 3, type, GL_FALSE, stride, p);
}

static inline void vgl_shim_ClientActiveTexture(GLenum unit)
{
	glClientActiveTexture(unit);
	vgl_shim_client_tex = unit;
}

static inline int vgl_shim_array_attr(GLenum array)
{
	switch (array) {
		case GL_VERTEX_ARRAY: return VGL_ATTR_VERTEX;
		case GL_TEXTURE_COORD_ARRAY: return (int)vgl_shim_tex_attr(vgl_shim_client_tex);
		case GL_COLOR_ARRAY: return VGL_ATTR_COLOR;
		case GL_NORMAL_ARRAY: return VGL_ATTR_NORMAL;
		default: return -1;
	}
}

static inline void vgl_shim_EnableClientState(GLenum array)
{
	glEnableClientState(array);
	int a = vgl_shim_array_attr(array);
	if (a >= 0) glEnableVertexAttribArray(a);
}

static inline void vgl_shim_DisableClientState(GLenum array)
{
	glDisableClientState(array);
	int a = vgl_shim_array_attr(array);
	if (a >= 0) glDisableVertexAttribArray(a);
}

static inline void vgl_shim_Color4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
	glColor4f(r, g, b, a);
	glVertexAttrib4f(VGL_ATTR_COLOR, r, g, b, a);
}
static inline void vgl_shim_Color3f(GLfloat r, GLfloat g, GLfloat b) { vgl_shim_Color4f(r, g, b, 1.0f); }
static inline void vgl_shim_Color4fv(const GLfloat *v) { vgl_shim_Color4f(v[0], v[1], v[2], v[3]); }
static inline void vgl_shim_Color3fv(const GLfloat *v) { vgl_shim_Color4f(v[0], v[1], v[2], 1.0f); }
static inline void vgl_shim_Color3us(GLushort r, GLushort g, GLushort b) { vgl_shim_Color4f(r / 65535.0f, g / 65535.0f, b / 65535.0f, 1.0f); }
static inline void vgl_shim_Color4us(GLushort r, GLushort g, GLushort b, GLushort a) { vgl_shim_Color4f(r / 65535.0f, g / 65535.0f, b / 65535.0f, a / 65535.0f); }
static inline void vgl_shim_Color3usv(const GLushort *c) { vgl_shim_Color3us(c[0], c[1], c[2]); }
static inline void vgl_shim_Color4usv(const GLushort *c) { vgl_shim_Color4us(c[0], c[1], c[2], c[3]); }
static inline void vgl_shim_Color4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a) { vgl_shim_Color4f(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f); }

static inline void vgl_shim_Normal3f(GLfloat x, GLfloat y, GLfloat z)
{
	glNormal3f(x, y, z);
	glVertexAttrib4f(VGL_ATTR_NORMAL, x, y, z, 0.0f);
}

static inline void vgl_shim_MultiTexCoord4f(GLenum unit, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
	if (unit == GL_TEXTURE0) {
		glVertexAttrib4f(VGL_ATTR_TEX0, s, t, r, q);
	} else {
		vgl_shim_tex1[0] = s; vgl_shim_tex1[1] = t; vgl_shim_tex1[2] = r; vgl_shim_tex1[3] = q;
		glVertexAttrib4f(VGL_ATTR_TEX1, s, t, r, q);
	}
}

static inline void vgl_shim_MultiTexCoord2f(GLenum unit, GLfloat s, GLfloat t)
{
	if (unit == GL_TEXTURE0) {
		glVertexAttrib4f(VGL_ATTR_TEX0, s, t, 0.0f, 1.0f);
	} else if (unit == GL_TEXTURE1) {
		vgl_shim_tex1[0] = s; vgl_shim_tex1[1] = t;
		glVertexAttrib4f(VGL_ATTR_TEX1, vgl_shim_tex1[0], vgl_shim_tex1[1], vgl_shim_tex1[2], vgl_shim_tex1[3]);
	} else {
		vgl_shim_tex1[2] = s; vgl_shim_tex1[3] = t;
		glVertexAttrib4f(VGL_ATTR_TEX1, vgl_shim_tex1[0], vgl_shim_tex1[1], vgl_shim_tex1[2], vgl_shim_tex1[3]);
	}
}

static inline void vgl_shim_Fogf(GLenum pname, GLfloat v)
{
	glFogf(pname, v);
	if (pname == GL_FOG_START) vgl_shim_fog_start = v;
	else if (pname == GL_FOG_END) vgl_shim_fog_end = v;
	else if (pname == GL_FOG_DENSITY) vgl_shim_fog_density = v;
}

static inline void vgl_shim_Fogfv(GLenum pname, const GLfloat *v)
{
	glFogfv(pname, v);
	if (pname == GL_FOG_COLOR) memcpy(vgl_shim_fog_color, v, sizeof(vgl_shim_fog_color));
	else if (pname == GL_FOG_START) vgl_shim_fog_start = v[0];
	else if (pname == GL_FOG_END) vgl_shim_fog_end = v[0];
	else if (pname == GL_FOG_DENSITY) vgl_shim_fog_density = v[0];
}

static inline void vgl_shim_UseProgram(GLuint prog)
{
	glUseProgram(prog);
	vgl_shim_program = prog;
}

typedef struct {
	GLuint prog;
	unsigned gen;
	GLint mvp, mv, mvinv, nm, tex, fogc, fogs, foge, fogd, rect, aref, gam, clip0, clip1, clip5;
	unsigned mat_gen;
	int mat_valid;
	GLfloat fog[7], rect_wh[2], aref_v, gam_v;
	unsigned clip_gen_v;
} vgl_shim_locs;

static inline vgl_shim_locs *vgl_shim_get_locs(GLuint prog)
{
	static vgl_shim_locs cache[64];
	static int count = 0;
	vgl_shim_locs *l = NULL;
	for (int i = 0; i < count; i++)
		if (cache[i].prog == prog) {
			if (cache[i].gen == vgl_shim_state.link_gen) return &cache[i];
			l = &cache[i];
			break;
		}
	if (!l) l = &cache[count < 64 ? count++ : 63];
	l->prog = prog;
	l->gen = vgl_shim_state.link_gen;
	l->mvp = glGetUniformLocation(prog, "vgl_MVP");
	l->mv = glGetUniformLocation(prog, "vgl_MV");
	l->mvinv = glGetUniformLocation(prog, "vgl_MVInv");
	l->nm = glGetUniformLocation(prog, "vgl_NormalMat");
	l->tex = glGetUniformLocation(prog, "vgl_TexMat0");
	l->fogc = glGetUniformLocation(prog, "vgl_FogColor");
	l->fogs = glGetUniformLocation(prog, "vgl_FogStart");
	l->foge = glGetUniformLocation(prog, "vgl_FogEnd");
	l->fogd = glGetUniformLocation(prog, "vgl_FogDensity");
	l->rect = glGetUniformLocation(prog, "vgl_RectSize");
	l->aref = glGetUniformLocation(prog, "vgl_AlphaRef");
	l->gam = glGetUniformLocation(prog, "vgl_Gamma");
	l->gam_v = -1.0f;
	l->clip0 = glGetUniformLocation(prog, "vgl_Clip0");
	l->clip1 = glGetUniformLocation(prog, "vgl_Clip1");
	l->clip5 = glGetUniformLocation(prog, "vgl_Clip5");
	l->clip_gen_v = 0;
	l->mat_valid = 0;
	l->fog[0] = -12345.0f; l->rect_wh[0] = -1.0f; l->aref_v = -12345.0f;
	return l;
}

static inline void vgl_shim_mul(GLfloat *out, const GLfloat *a, const GLfloat *b)
{
	for (int c = 0; c < 4; c++)
		for (int r = 0; r < 4; r++) {
			GLfloat s = 0.0f;
			for (int k = 0; k < 4; k++) s += a[k * 4 + r] * b[c * 4 + k];
			out[c * 4 + r] = s;
		}
}

static inline int vgl_shim_invert(const GLfloat *m, GLfloat *inv)
{
	GLfloat t[16];
	t[0]  =  m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15] + m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
	t[4]  = -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15] - m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
	t[8]  =  m[4]*m[9]*m[15]  - m[4]*m[11]*m[13] - m[8]*m[5]*m[15] + m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
	t[12] = -m[4]*m[9]*m[14]  + m[4]*m[10]*m[13] + m[8]*m[5]*m[14] - m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];
	t[1]  = -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15] - m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
	t[5]  =  m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15] + m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
	t[9]  = -m[0]*m[9]*m[15]  + m[0]*m[11]*m[13] + m[8]*m[1]*m[15] - m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
	t[13] =  m[0]*m[9]*m[14]  - m[0]*m[10]*m[13] - m[8]*m[1]*m[14] + m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];
	t[2]  =  m[1]*m[6]*m[15]  - m[1]*m[7]*m[14]  - m[5]*m[2]*m[15] + m[5]*m[3]*m[14] + m[13]*m[2]*m[7]  - m[13]*m[3]*m[6];
	t[6]  = -m[0]*m[6]*m[15]  + m[0]*m[7]*m[14]  + m[4]*m[2]*m[15] - m[4]*m[3]*m[14] - m[12]*m[2]*m[7]  + m[12]*m[3]*m[6];
	t[10] =  m[0]*m[5]*m[15]  - m[0]*m[7]*m[13]  - m[4]*m[1]*m[15] + m[4]*m[3]*m[13] + m[12]*m[1]*m[7]  - m[12]*m[3]*m[5];
	t[14] = -m[0]*m[5]*m[14]  + m[0]*m[6]*m[13]  + m[4]*m[1]*m[14] - m[4]*m[2]*m[13] - m[12]*m[1]*m[6]  + m[12]*m[2]*m[5];
	t[3]  = -m[1]*m[6]*m[11]  + m[1]*m[7]*m[10]  + m[5]*m[2]*m[11] - m[5]*m[3]*m[10] - m[9]*m[2]*m[7]   + m[9]*m[3]*m[6];
	t[7]  =  m[0]*m[6]*m[11]  - m[0]*m[7]*m[10]  - m[4]*m[2]*m[11] + m[4]*m[3]*m[10] + m[8]*m[2]*m[7]   - m[8]*m[3]*m[6];
	t[11] = -m[0]*m[5]*m[11]  + m[0]*m[7]*m[9]   + m[4]*m[1]*m[11] - m[4]*m[3]*m[9]  - m[8]*m[1]*m[7]   + m[8]*m[3]*m[5];
	t[15] =  m[0]*m[5]*m[10]  - m[0]*m[6]*m[9]   - m[4]*m[1]*m[10] + m[4]*m[2]*m[9]  + m[8]*m[1]*m[6]   - m[8]*m[2]*m[5];
	GLfloat det = m[0] * t[0] + m[1] * t[4] + m[2] * t[8] + m[3] * t[12];
	if (det == 0.0f) return 0;
	det = 1.0f / det;
	for (int i = 0; i < 16; i++) inv[i] = t[i] * det;
	return 1;
}

static inline void vgl_shim_prepare_draw(void)
{
	if (!vgl_shim_program) return;
	if (vgl_shim_state.alpha_twin) {
		GLboolean at = glIsEnabled(GL_ALPHA_TEST);
		if (at && vgl_shim_program == vgl_shim_state.alpha_base) {
			glUseProgram(vgl_shim_state.alpha_twin);
			vgl_shim_program = vgl_shim_state.alpha_twin;
			vgl_shim_state.perf_switch++;
		} else if (!at && vgl_shim_program == vgl_shim_state.alpha_twin) {
			glUseProgram(vgl_shim_state.alpha_base);
			vgl_shim_program = vgl_shim_state.alpha_base;
			vgl_shim_state.perf_switch++;
		}
	}
	vgl_shim_locs *l = vgl_shim_get_locs(vgl_shim_program);

	if (!l->mat_valid || l->mat_gen != vgl_shim_state.mat_gen) {
		GLfloat mv[16], pr[16], tmp[16];
		l->mat_valid = 1;
		l->mat_gen = vgl_shim_state.mat_gen;
		vgl_shim_state.perf_mat++;
		if (l->mvp >= 0 || l->mv >= 0 || l->mvinv >= 0 || l->nm >= 0) {
			glGetFloatv(GL_MODELVIEW_MATRIX, mv);
			if (l->mv >= 0) glUniformMatrix4fv(l->mv, 1, GL_FALSE, mv);
			if (l->mvp >= 0) {
				glGetFloatv(GL_PROJECTION_MATRIX, pr);
				vgl_shim_mul(tmp, pr, mv);
				glUniformMatrix4fv(l->mvp, 1, GL_FALSE, tmp);
			}
			if ((l->mvinv >= 0 || l->nm >= 0) && vgl_shim_invert(mv, tmp)) {
				if (l->mvinv >= 0) glUniformMatrix4fv(l->mvinv, 1, GL_FALSE, tmp);
				if (l->nm >= 0) {
					GLfloat n[9] = { tmp[0], tmp[4], tmp[8], tmp[1], tmp[5], tmp[9], tmp[2], tmp[6], tmp[10] };
					glUniformMatrix3fv(l->nm, 1, GL_FALSE, n);
				}
			}
		}
		if (l->tex >= 0) {
			glGetFloatv(GL_TEXTURE_MATRIX, tmp);
			glUniformMatrix4fv(l->tex, 1, GL_FALSE, tmp);
		}
	}
	if (l->fogc >= 0 || l->fogs >= 0 || l->foge >= 0 || l->fogd >= 0) {
		GLfloat f[7] = { vgl_shim_fog_color[0], vgl_shim_fog_color[1], vgl_shim_fog_color[2], vgl_shim_fog_color[3],
		                 vgl_shim_fog_start, vgl_shim_fog_end, vgl_shim_fog_density };
		if (memcmp(f, l->fog, sizeof(f))) {
			memcpy(l->fog, f, sizeof(f));
			if (l->fogc >= 0) glUniform4fv(l->fogc, 1, f);
			if (l->fogs >= 0) glUniform1f(l->fogs, f[4]);
			if (l->foge >= 0) glUniform1f(l->foge, f[5]);
			if (l->fogd >= 0) glUniform1f(l->fogd, f[6]);
		}
	}
	if ((l->clip0 >= 0 || l->clip1 >= 0 || l->clip5 >= 0) && l->clip_gen_v != vgl_shim_state.clip_gen) {
		static const GLfloat keep[4] = { 0, 0, 0, 1 };
		l->clip_gen_v = vgl_shim_state.clip_gen;
		if (l->clip0 >= 0) glUniform4fv(l->clip0, 1, (vgl_shim_state.clip_mask & 1) ? vgl_shim_state.clip[0] : keep);
		if (l->clip1 >= 0) glUniform4fv(l->clip1, 1, (vgl_shim_state.clip_mask & 2) ? vgl_shim_state.clip[1] : keep);
		if (l->clip5 >= 0) glUniform4fv(l->clip5, 1, (vgl_shim_state.clip_mask & 32) ? vgl_shim_state.clip[5] : keep);
	}
	if (l->gam >= 0 && vgl_shim_state.gamma != l->gam_v) {
		l->gam_v = vgl_shim_state.gamma;
		glUniform1f(l->gam, l->gam_v);
	}
	if (l->aref >= 0) {
		GLfloat ref = -1.0f;
		if (glIsEnabled(GL_ALPHA_TEST)) glGetFloatv(GL_ALPHA_TEST_REF, &ref);
		if (ref != l->aref_v) { l->aref_v = ref; glUniform1f(l->aref, ref); }
	}
	if (l->rect >= 0) {
		GLint vp[4];
		glGetIntegerv(GL_VIEWPORT, vp);
		if ((GLfloat)vp[2] != l->rect_wh[0] || (GLfloat)vp[3] != l->rect_wh[1]) {
			l->rect_wh[0] = (GLfloat)vp[2]; l->rect_wh[1] = (GLfloat)vp[3];
			glUniform2f(l->rect, l->rect_wh[0], l->rect_wh[1]);
		}
	}
}

#if VGL_SHIM_PERF
static inline void vgl_shim_perf_tick(SceUInt64 t0, SceUInt64 t1, SceUInt64 t2)
{
	vgl_shim_state.perf_draws++;
	vgl_shim_state.perf_us_prep += t1 - t0;
	vgl_shim_state.perf_us_draw += t2 - t1;
	if (!vgl_shim_state.perf_last) vgl_shim_state.perf_last = t2;
	if (t2 - vgl_shim_state.perf_last >= 1000000) {
		FILE *f = fopen("ux0:/data/AlephOne/glperf_log.txt", "a");
		if (f) {
			fprintf(f, "draws %5u  matrix uploads %5u  switches %4u  prep %4u ms  draw %4u ms\n",
				vgl_shim_state.perf_draws, vgl_shim_state.perf_mat, vgl_shim_state.perf_switch,
				(unsigned)(vgl_shim_state.perf_us_prep / 1000), (unsigned)(vgl_shim_state.perf_us_draw / 1000));
			fclose(f);
		}
		vgl_shim_state.perf_draws = vgl_shim_state.perf_mat = vgl_shim_state.perf_switch = 0;
		vgl_shim_state.perf_us_prep = vgl_shim_state.perf_us_draw = 0;
		vgl_shim_state.perf_last = t2;
	}
}
#define VGL_SHIM_TIMED(call) do { \
	SceUInt64 _t0 = sceKernelGetProcessTimeWide(); \
	vgl_shim_prepare_draw(); \
	SceUInt64 _t1 = sceKernelGetProcessTimeWide(); \
	call; \
	vgl_shim_perf_tick(_t0, _t1, sceKernelGetProcessTimeWide()); \
} while (0)
#else
#define VGL_SHIM_TIMED(call) do { vgl_shim_prepare_draw(); call; } while (0)
#endif

static inline void vgl_shim_DrawArrays(GLenum mode, GLint first, GLsizei count)
{
	VGL_SHIM_TIMED(glDrawArrays(mode, first, count));
}

static inline void vgl_shim_DrawElements(GLenum mode, GLsizei count, GLenum type, const void *idx)
{
	VGL_SHIM_TIMED(glDrawElements(mode, count, type, idx));
}

static inline void vgl_shim_ClipPlane(GLenum plane, const GLdouble *eq)
{
	glClipPlane(plane, eq);
	int i = (int)(plane - GL_CLIP_PLANE0);
	if (i < 0 || i >= 6) return;
	GLfloat mv[16], inv[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, mv);
	if (!vgl_shim_invert(mv, inv)) return;
	for (int c = 0; c < 4; c++)
		vgl_shim_state.clip[i][c] = (GLfloat)(eq[0] * inv[c * 4 + 0] + eq[1] * inv[c * 4 + 1] + eq[2] * inv[c * 4 + 2] + eq[3] * inv[c * 4 + 3]);
	vgl_shim_state.clip_gen++;
}

static inline void vgl_shim_track_clip(GLenum cap, int on)
{
	if (cap >= GL_CLIP_PLANE0 && cap <= GL_CLIP_PLANE0 + 5) {
		unsigned bit = 1u << (cap - GL_CLIP_PLANE0);
		unsigned m = on ? (vgl_shim_state.clip_mask | bit) : (vgl_shim_state.clip_mask & ~bit);
		if (m != vgl_shim_state.clip_mask) { vgl_shim_state.clip_mask = m; vgl_shim_state.clip_gen++; }
	}
}
static inline void vgl_shim_Enable(GLenum cap) { glEnable(cap); vgl_shim_track_clip(cap, 1); }
static inline void vgl_shim_Disable(GLenum cap) { glDisable(cap); vgl_shim_track_clip(cap, 0); }

static inline void vgl_shim_LinkProgram(GLuint prog)
{
	glLinkProgram(prog);
	vgl_shim_state.link_gen++;
}

#define glLinkProgram          vgl_shim_LinkProgram
#define glClipPlane            vgl_shim_ClipPlane
#define glEnable               vgl_shim_Enable
#define glDisable              vgl_shim_Disable

#define glLoadIdentity(...) (glLoadIdentity(__VA_ARGS__), (void)++vgl_shim_state.mat_gen)
#define glLoadMatrixf(...) (glLoadMatrixf(__VA_ARGS__), (void)++vgl_shim_state.mat_gen)
#define glLoadMatrixd(...) (glLoadMatrixd(__VA_ARGS__), (void)++vgl_shim_state.mat_gen)
#define glMultMatrixf(...) (glMultMatrixf(__VA_ARGS__), (void)++vgl_shim_state.mat_gen)
#define glMultMatrixd(...) (glMultMatrixd(__VA_ARGS__), (void)++vgl_shim_state.mat_gen)
#define glTranslatef(...) (glTranslatef(__VA_ARGS__), (void)++vgl_shim_state.mat_gen)
#define glTranslated(...) (glTranslated(__VA_ARGS__), (void)++vgl_shim_state.mat_gen)
#define glRotatef(...) (glRotatef(__VA_ARGS__), (void)++vgl_shim_state.mat_gen)
#define glRotated(...) (glRotated(__VA_ARGS__), (void)++vgl_shim_state.mat_gen)
#define glScalef(...) (glScalef(__VA_ARGS__), (void)++vgl_shim_state.mat_gen)
#define glScaled(...) (glScaled(__VA_ARGS__), (void)++vgl_shim_state.mat_gen)
#define glPopMatrix(...) (glPopMatrix(__VA_ARGS__), (void)++vgl_shim_state.mat_gen)
#define glOrtho(...) (glOrtho(__VA_ARGS__), (void)++vgl_shim_state.mat_gen)
#define glFrustum(...) (glFrustum(__VA_ARGS__), (void)++vgl_shim_state.mat_gen)
#define gluPerspective(...) (gluPerspective(__VA_ARGS__), (void)++vgl_shim_state.mat_gen)

#define glVertexPointer        vgl_shim_VertexPointer
#define glTexCoordPointer      vgl_shim_TexCoordPointer
#define glColorPointer         vgl_shim_ColorPointer
#define glNormalPointer        vgl_shim_NormalPointer
#define glClientActiveTexture  vgl_shim_ClientActiveTexture
#define glEnableClientState    vgl_shim_EnableClientState
#define glDisableClientState   vgl_shim_DisableClientState
#define glColor4f              vgl_shim_Color4f
#define glColor3f              vgl_shim_Color3f
#define glColor4fv             vgl_shim_Color4fv
#define glColor3fv             vgl_shim_Color3fv
#define glColor4ub             vgl_shim_Color4ub
#define glColor3us             vgl_shim_Color3us
#define glColor4us             vgl_shim_Color4us
#define glColor3usv            vgl_shim_Color3usv
#define glColor4usv            vgl_shim_Color4usv
#define glNormal3f             vgl_shim_Normal3f
#define glMultiTexCoord4f      vgl_shim_MultiTexCoord4f
#define glMultiTexCoord2f      vgl_shim_MultiTexCoord2f
#define glFogf                 vgl_shim_Fogf
#define glFogfv                vgl_shim_Fogfv
#define glUseProgram           vgl_shim_UseProgram
#define glDrawArrays           vgl_shim_DrawArrays
#define glDrawElements         vgl_shim_DrawElements

#endif
#endif
