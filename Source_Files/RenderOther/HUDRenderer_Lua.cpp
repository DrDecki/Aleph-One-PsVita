/*
HUD_RENDERER_LUA.CPP

    Copyright (C) 2009 by Jeremiah Morris and the Aleph One developers

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    This license is contained in the file "COPYING",
    which is included with this source code; it is available online at
    http://www.gnu.org/licenses/gpl.html

    Implements HUD helper class for Lua HUD themes
*/

#include "HUDRenderer_Lua.h"
#include <stdio.h>

#include "FontHandler.h"
#include "Image_Blitter.h"
#include "Shape_Blitter.h"

#include "lua_hud_script.h"
#include "shell.h"
#include "screen.h"
#include "images.h"

#ifdef HAVE_OPENGL
#include "OGL_Headers.h"
#include "OGL_Render.h"
#endif

#include <math.h>

extern bool MotionSensorActive;


// Rendering object
static HUD_Lua_Class HUD_Lua;

#ifdef __vita__
static SDL_Rect g_hud_dirty = {0,0,0,0};
static bool g_hud_dirty_valid = false;
static void hud_mark_dirty(const SDL_Rect &r) {
	if (r.w <= 0 || r.h <= 0) return;
	if (!g_hud_dirty_valid) { g_hud_dirty = r; g_hud_dirty_valid = true; return; }
	int x0 = g_hud_dirty.x < r.x ? g_hud_dirty.x : r.x;
	int y0 = g_hud_dirty.y < r.y ? g_hud_dirty.y : r.y;
	int x1 = (g_hud_dirty.x+g_hud_dirty.w) > (r.x+r.w) ? (g_hud_dirty.x+g_hud_dirty.w) : (r.x+r.w);
	int y1 = (g_hud_dirty.y+g_hud_dirty.h) > (r.y+r.h) ? (g_hud_dirty.y+g_hud_dirty.h) : (r.y+r.h);
	g_hud_dirty.x = x0; g_hud_dirty.y = y0; g_hud_dirty.w = x1-x0; g_hud_dirty.h = y1-y0;
}
#endif


HUD_Lua_Class *Lua_HUDInstance()
{
	return &HUD_Lua;
}

void Lua_DrawHUD(short time_elapsed)
{
#ifdef VITA_PERF_LOG
	static double _h_ms=0, _h_sd=0, _h_dr=0; static int _h_fc=0;
	Uint64 _hf = SDL_GetPerformanceFrequency();
	Uint64 _t0 = SDL_GetPerformanceCounter();
	HUD_Lua.update_motion_sensor(time_elapsed);
	Uint64 _t1 = SDL_GetPerformanceCounter();
	HUD_Lua.start_draw();
	Uint64 _t2 = SDL_GetPerformanceCounter();
	L_Call_HUDDraw();
	Uint64 _t3 = SDL_GetPerformanceCounter();
	HUD_Lua.end_draw();
	_h_ms += (double)(_t1-_t0)/_hf*1000.0;
	_h_sd += (double)(_t2-_t1)/_hf*1000.0;
	_h_dr += (double)(_t3-_t2)/_hf*1000.0;
	if (++_h_fc >= 120) {
		FILE *_hfp = fopen("ux0:/hud.txt", "w");
		if (_hfp) {
			fprintf(_hfp, "motion_sensor: %.2f ms\n", _h_ms/_h_fc);
			fprintf(_hfp, "start_draw(Clear): %.2f ms\n", _h_sd/_h_fc);
			fprintf(_hfp, "L_Call_HUDDraw(Blits): %.2f ms\n", _h_dr/_h_fc);
			fclose(_hfp);
		}
		_h_ms=_h_sd=_h_dr=0; _h_fc=0;
	}
#else
	HUD_Lua.update_motion_sensor(time_elapsed);
	HUD_Lua.start_draw();
	L_Call_HUDDraw();
	HUD_Lua.end_draw();
#endif
}

/*
 *  Update motion sensor
 */

void HUD_Lua_Class::update_motion_sensor(short time_elapsed)
{
	render_motion_sensor(time_elapsed);
}

void HUD_Lua_Class::clear_entity_blips(void)
{
	m_blips.clear();
}

void HUD_Lua_Class::add_entity_blip(short mtype, short intensity, short x, short y)
{
	world_point2d origin, target;
	origin.x = origin.y = 0;
	target.x = x;
	target.y = y;
	
	blip_info info;
	info.mtype = mtype;
	info.intensity = intensity;
	info.distance = distance2d(&origin, &target);
	info.direction = arctangent(x, y);
	
	m_blips.push_back(info);
}

size_t HUD_Lua_Class::entity_blip_count(void)
{
	return m_blips.size();
}

blip_info HUD_Lua_Class::entity_blip(size_t index)
{
	return m_blips[index];
}

void HUD_Lua_Class::start_draw(void)
{
	alephone::Screen *scr = alephone::Screen::instance();
	scr->bound_screen();
    m_wr = scr->window_rect();
	m_opengl = (get_screen_mode()->acceleration != _no_acceleration);
	m_masking_mode = _mask_disabled;
	
#ifdef HAVE_OPENGL
	if (m_opengl)
	{
		glPushAttrib(GL_ALL_ATTRIB_BITS);
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_STENCIL_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_FOG);
        
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glTranslatef(m_wr.x, m_wr.y, 0.0);
		
		m_surface = NULL;
	}
	else
#endif
	{
		if (m_surface &&
				(m_surface->w != MainScreenLogicalWidth() ||
				 m_surface->h != MainScreenLogicalHeight()))
		{
			SDL_FreeSurface(m_surface);
			m_surface = NULL;
		}
		if (!m_surface)
		{
			m_surface = SDL_ConvertSurfaceFormat(MainScreenSurface(), SDL_PIXELFORMAT_ARGB8888, 0);
			SDL_SetSurfaceBlendMode(m_surface, SDL_BLENDMODE_BLEND);
		}
#ifdef __vita__
		if (g_hud_dirty_valid) {
			SDL_Rect _c = g_hud_dirty;
			if (_c.x < 0) { _c.w += _c.x; _c.x = 0; }
			if (_c.y < 0) { _c.h += _c.y; _c.y = 0; }
			if (_c.x + _c.w > m_surface->w) _c.w = m_surface->w - _c.x;
			if (_c.y + _c.h > m_surface->h) _c.h = m_surface->h - _c.y;
			if (_c.w > 0 && _c.h > 0) SDL_FillRect(m_surface, &_c, SDL_MapRGBA(m_surface->format, 0, 0, 0, 0));
		} else {
			SDL_FillRect(m_surface, NULL, SDL_MapRGBA(m_surface->format, 0, 0, 0, 0));
		}
		g_hud_dirty_valid = false;
#else
		SDL_FillRect(m_surface, NULL, SDL_MapRGBA(m_surface->format, 0, 0, 0, 0));	
#endif
	}
	
	
	m_drawing = true;
	clear_mask();
}

void HUD_Lua_Class::end_draw(void)
{
	m_drawing = false;
	
#ifdef HAVE_OPENGL
	if (m_opengl)
	{
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
		glPopAttrib();
	}
#endif
}

void HUD_Lua_Class::apply_clip(void)
{
	alephone::Screen *scr = alephone::Screen::instance();
	
    SDL_Rect r;
    r.x = m_wr.x + scr->lua_clip_rect.x;
    r.y = m_wr.y + scr->lua_clip_rect.y;
    r.w = MIN(scr->lua_clip_rect.w, m_wr.w - scr->lua_clip_rect.x);
    r.h = MIN(scr->lua_clip_rect.h, m_wr.h - scr->lua_clip_rect.y);
#ifdef HAVE_OPENGL
	if (m_opengl)
	{
		glEnable(GL_SCISSOR_TEST);
		scr->scissor_screen_to_rect(r);
	}
	else
#endif
	if (m_surface)
	{
		SDL_SetClipRect(MainScreenSurface(), &r);
	}
}

short HUD_Lua_Class::masking_mode(void)
{
	return m_masking_mode;
}

void HUD_Lua_Class::set_masking_mode(short masking_mode)
{
	if (m_masking_mode == masking_mode ||
		masking_mode < 0 ||
		masking_mode >= NUMBER_OF_LUA_MASKING_MODES)
		return;
	
	if (m_masking_mode == _mask_drawing)
		end_drawing_mask();
	else if (m_masking_mode == _mask_erasing)
		end_drawing_mask();
	else if (m_masking_mode == _mask_enabled)
		end_using_mask();
	
	m_masking_mode = masking_mode;
	if (m_masking_mode == _mask_drawing)
		start_drawing_mask(false);
	else if (m_masking_mode == _mask_erasing)
		start_drawing_mask(true);
	else if (m_masking_mode == _mask_enabled)
		start_using_mask();
}
	
void HUD_Lua_Class::clear_mask(void)
{
	if (!m_drawing)
		return;
	
#ifdef HAVE_OPENGL
	if (m_opengl)
	{
		glClearStencil(0);
		glClear(GL_STENCIL_BUFFER_BIT);
	}
#endif
}

void HUD_Lua_Class::start_using_mask(void)
{
#ifdef HAVE_OPENGL
	if (m_opengl)
	{
		glEnable(GL_STENCIL_TEST);
		glStencilFunc(GL_EQUAL, 1, 1);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	}
#endif
}

void HUD_Lua_Class::end_using_mask(void)
{
#ifdef HAVE_OPENGL
	if (m_opengl)
	{
		glDisable(GL_STENCIL_TEST);
	}
#endif
}

void HUD_Lua_Class::start_drawing_mask(bool erase)
{
#ifdef HAVE_OPENGL
	if (m_opengl)
	{
		glEnable(GL_STENCIL_TEST);
		glStencilFunc(GL_ALWAYS, erase ? 0 : 1, 1);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.5);
		
		glColorMask(false, false, false, false);
	}
#endif
}

void HUD_Lua_Class::end_drawing_mask(void)
{
#ifdef HAVE_OPENGL
	if (m_opengl)
	{
		glDisable(GL_STENCIL_TEST);
		glDisable(GL_ALPHA_TEST);
		glColorMask(true, true, true, true);
	}
#endif
}

void HUD_Lua_Class::fill_rect(float x, float y, float w, float h,
															float r, float g, float b, float a)
{
	if (!m_drawing)
		return;
	
	if (!w || !h)
		return;
	
	apply_clip();
#ifdef HAVE_OPENGL
	if (m_opengl)
	{
		glColor4f(r, g, b, a);
		OGL_RenderRect(x, y, w, h);
	}
	else
#endif
	if (m_surface)
	{
		SDL_Rect rect;
		rect.x = static_cast<Sint16>(x) + m_wr.x;
		rect.y = static_cast<Sint16>(y) + m_wr.y;
		rect.w = static_cast<Uint16>(w);
		rect.h = static_cast<Uint16>(h);
		SDL_FillRect(m_surface, &rect,
								 SDL_MapRGBA(m_surface->format, static_cast<unsigned char>(r * 255), static_cast<unsigned char>(g * 255), static_cast<unsigned char>(b * 255), static_cast<unsigned char>(a * 255)));
		SDL_BlitSurface(m_surface, &rect, MainScreenSurface(), &rect);
#ifdef __vita__
		hud_mark_dirty(rect);
#endif
		SDL_SetClipRect(MainScreenSurface(), NULL);
	}
}	

void HUD_Lua_Class::frame_rect(float x, float y, float w, float h,
												 			 float r, float g, float b, float a,
															 float t)
{
	if (!m_drawing)
		return;
		
	apply_clip();
#ifdef HAVE_OPENGL
	if (m_opengl)
	{
		glColor4f(r, g, b, a);
		OGL_RenderFrame(x, y, w, h, t);
	}
	else
#endif
	if (m_surface)
	{
		Uint32 color = SDL_MapRGBA(m_surface->format, static_cast<unsigned char>(r * 255), static_cast<unsigned char>(g * 255), static_cast<unsigned char>(b * 255), static_cast<unsigned char>(a * 255));
		SDL_Rect rect;
		rect.x = static_cast<Sint16>(x) + m_wr.x;
		rect.w = static_cast<Uint16>(w);
		rect.y = static_cast<Sint16>(y) + m_wr.y;
		rect.h = static_cast<Uint16>(t);
		SDL_FillRect(m_surface, &rect, color);
		SDL_BlitSurface(m_surface, &rect, MainScreenSurface(), &rect);
#ifdef __vita__
		hud_mark_dirty(rect);
#endif
		rect.x = static_cast<Sint16>(x) + m_wr.x;
		rect.w = static_cast<Uint16>(w);
		rect.y = static_cast<Sint16>(y + h - t) + m_wr.y;
		rect.h = static_cast<Uint16>(t);
		SDL_FillRect(m_surface, &rect, color);
		SDL_BlitSurface(m_surface, &rect, MainScreenSurface(), &rect);
#ifdef __vita__
		hud_mark_dirty(rect);
#endif
		rect.x = static_cast<Sint16>(x) + m_wr.x;
		rect.w = static_cast<Uint16>(t);
		rect.y = static_cast<Sint16>(y + t) + m_wr.y;
		rect.h = static_cast<Uint16>(h - t - t);
		SDL_FillRect(m_surface, &rect, color);
		SDL_BlitSurface(m_surface, &rect, MainScreenSurface(), &rect);
#ifdef __vita__
		hud_mark_dirty(rect);
#endif
		rect.x = static_cast<Sint16>(x + w - t) + m_wr.x;
		rect.w = static_cast<Uint16>(t);
		rect.y = static_cast<Sint16>(y + t) + m_wr.y;
		rect.h = static_cast<Uint16>(h - t - t);
		SDL_FillRect(m_surface, &rect, color);
		SDL_BlitSurface(m_surface, &rect, MainScreenSurface(), &rect);
#ifdef __vita__
		hud_mark_dirty(rect);
#endif
		SDL_SetClipRect(MainScreenSurface(), NULL);
	}
}	

void HUD_Lua_Class::draw_text(FontSpecifier *font, const char *text,
															float x, float y,
															float r, float g, float b, float a,
															float scale)
{
	if (!m_drawing)
		return;
	
	if (!text || !strlen(text))
		return;
	
	apply_clip();
#ifdef HAVE_OPENGL
	if (m_opengl)
	{
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glTranslatef(x, y + (font->Height * scale), 0);
        glScalef(scale, scale, 1.0);
		glColor4f(r, g, b, a);
		font->OGL_Render(text);
		glColor4f(1, 1, 1, 1);
		glPopMatrix();
	}
	else
#endif
	if (m_surface)
	{
		SDL_Rect rect;
		rect.x = static_cast<Sint16>(x) + m_wr.x;
		rect.y = static_cast<Sint16>(y) + m_wr.y;
		rect.w = font->TextWidth(text);
		rect.h = font->LineSpacing;
        
        // FIXME: draw_text doesn't support full RGBA transfer for proper scaling,
        // so draw blended but unscaled text instead
#if 0
        if (scale < 0.99 || scale > 1.01)
        {
            SDL_Surface *s2 = SDL_CreateRGBSurface(SDL_SWSURFACE, rect.w, rect.h, m_surface->format->BitsPerPixel, m_surface->format->Rmask, m_surface->format->Gmask, m_surface->format->Bmask, m_surface->format->Amask);
            
            font->Info->draw_text(s2, text, strlen(text),
                                  0, font->Height,
                                  SDL_MapRGBA(m_surface->format,
                                              static_cast<unsigned char>(r * 255),
                                              static_cast<unsigned char>(g * 255),
                                              static_cast<unsigned char>(b * 255),
                                              static_cast<unsigned char>(a * 255)),
                                  font->Style);
            
            SDL_Rect srcrect;
            srcrect.x = 0;
            srcrect.y = 0;
            srcrect.w = ceilf(rect.w * scale);
            srcrect.h = ceilf(rect.h * scale);
            SDL_Surface *s3 = rescale_surface(s2, srcrect.w, srcrect.h);
            SDL_FreeSurface(s2);
            
            rect.w = srcrect.w;
            rect.h = srcrect.h;
            SDL_BlitSurface(s3, &srcrect, MainScreenSurface(), &rect);
            SDL_FreeSurface(s3);
        }
        else
#endif
        {
            SDL_BlitSurface(MainScreenSurface(), &rect, m_surface, &rect);
            font->Info->draw_text(m_surface, text, strlen(text),
                                  rect.x, rect.y + font->Height,
                                  SDL_MapRGBA(m_surface->format,
                                              static_cast<unsigned char>(r * 255),
                                              static_cast<unsigned char>(g * 255),
                                              static_cast<unsigned char>(b * 255),
                                              static_cast<unsigned char>(a * 255)),
                                  font->Style);
            SDL_BlitSurface(m_surface, &rect, MainScreenSurface(), &rect);
#ifdef __vita__
            hud_mark_dirty(rect);
#endif
			SDL_SetClipRect(MainScreenSurface(), NULL);
        }
	}
}

void HUD_Lua_Class::draw_image(Image_Blitter *image, float x, float y)
{
	if (!m_drawing)
		return;
	
	Image_Rect r{ x, y, image->crop_rect.w, image->crop_rect.h };
	
	if (!r.w || !r.h)
		return;

	apply_clip();
    if (m_surface)
    {
        r.x += m_wr.x;
        r.y += m_wr.y;
    }
	image->Draw(MainScreenSurface(), r);
#ifdef __vita__
	{ SDL_Rect _ir = { (int)r.x, (int)r.y, (int)r.w, (int)r.h }; hud_mark_dirty(_ir); }
#endif
	SDL_SetClipRect(MainScreenSurface(), NULL);
}

void HUD_Lua_Class::draw_shape(Shape_Blitter *shape, float x, float y)
{
	if (!m_drawing)
		return;
	
	Image_Rect r;
	r.x = x;
	r.y = y;
	r.w = shape->crop_rect.w;
	r.h = shape->crop_rect.h;
	
	if (!r.w || !r.h)
		return;
    
	apply_clip();
#ifdef HAVE_OPENGL
    if (m_opengl)
    {
        shape->OGL_Draw(r);
    }
    else
#endif
    if (m_surface)
    {
        r.x += m_wr.x;
        r.y += m_wr.y;
        shape->SDL_Draw(MainScreenSurface(), r);
#ifdef __vita__
        { SDL_Rect _sr = { (int)r.x, (int)r.y, (int)r.w, (int)r.h }; hud_mark_dirty(_sr); }
#endif
		SDL_SetClipRect(MainScreenSurface(), NULL);
    }
}
