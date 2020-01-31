/*  DreamChess
**
**  DreamChess is the legal property of its developers, whose names are too
**  numerous to list here. Please refer to the AUTHORS.txt file distributed
**  with this source distribution.
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <GL/glew.h>
#include <string.h>

#include "debug.h"
#include "ui_sdlgl.h"
#include "unicode.h"
#include "dir.h"

#include "freetype-gl/texture-font.h"

static texture_atlas_t *atlas;
static texture_font_t *font;
static unsigned int gpu_atlas_size;

typedef struct {
	float x, y, z;
	float s, t;
} vertex_t;

static int load_font(float pt_size) {
	ch_datadir();
	char *font_path = dir_get_real_path("fonts/OpenSans-Regular.ttf");

	if (!font_path) {
		DBG_ERROR("Failed to get full path to font file");
		return -1;
	}

	font = texture_font_new_from_file(atlas, pt_size, font_path);

	free(font_path);

	if (!font) {
		DBG_ERROR("Failed to load font");
		return -1;
	}

	return 0;
}

int unicode_init(float pt_size) {
	atlas  = texture_atlas_new(1024, 1024, 1);

	glGenTextures(1, &atlas->id);
	glBindTexture(GL_TEXTURE_2D, atlas->id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, atlas->width, atlas->height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, atlas->data);

	return load_font(pt_size);
}

int unicode_resize(float pt_size) {
	texture_atlas_clear(atlas);
	gpu_atlas_size = 0;
	texture_font_delete(font);
	return load_font(pt_size);
}

void unicode_exit(void) {
	texture_font_delete(font);

	glDeleteTextures(1, &atlas->id);
	texture_atlas_delete(atlas);
}

static vertex_t *create_vertex_array(const char *text, size_t *array_size) {
	const size_t text_len = strlen(text);
	*array_size = text_len * 4;
	vertex_t *vertex_array = malloc(sizeof(vertex_t) * *array_size);

	float pen_x = 0.0f;
	const float pen_y = 0.0f;
	for (size_t i = 0; i < text_len; ++i) {
		texture_glyph_t *glyph = texture_font_get_glyph(font, text + i);

		if (glyph != NULL) {
			float kerning = 0.0f;
			if (i > 0)
				kerning = texture_glyph_get_kerning(glyph, text + i - 1);

			pen_x += kerning;

			const int x0  = (int)(pen_x + glyph->offset_x);
			const int y0  = (int)(pen_y + glyph->offset_y);
			const int x1  = (int)(x0 + glyph->width);
			const int y1  = (int)(y0 - glyph->height);

			vertex_array[i * 4] = (vertex_t){ x0, y0, 1.0f, glyph->s0, glyph->t0 };
			vertex_array[i * 4 + 1] = (vertex_t){ x0, y1, 1.0f, glyph->s0, glyph->t1 };
			vertex_array[i * 4 + 2] = (vertex_t){ x1, y1, 1.0f, glyph->s1, glyph->t1 };
			vertex_array[i * 4 + 3] = (vertex_t){ x1, y0, 1.0f, glyph->s1, glyph->t0 };

			pen_x += glyph->advance_x;
		} else {
			DBG_LOG("Failed to load glyph for %c", text[i]);
		}
	}

	if (vector_size(font->glyphs) > gpu_atlas_size) {
		glBindTexture(GL_TEXTURE_2D, atlas->id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, atlas->width, atlas->height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, atlas->data);
		gpu_atlas_size = vector_size(font->glyphs);
	}

	return vertex_array;
}

static void render_vertex_array(const vertex_t *vertex_array, size_t size, float x, float y, float scale, gg_colour_t colour) {
	glEnable(GL_TEXTURE_2D);

	glColor4f(colour.r, colour.g, colour.b, colour.a);
	glBindTexture(GL_TEXTURE_2D, atlas->id);

	glPushMatrix();
	glTranslatef(x, y, 0.0f);
	glScalef(scale, scale, 1.0f);
	glBegin(GL_QUADS);

	for (size_t i = 0; i < size; ++i) {
		glTexCoord2f(vertex_array[i].s, vertex_array[i].t);
		glVertex3f(vertex_array[i].x, vertex_array[i].y, vertex_array[i].z);
	}

	glEnd();
	glPopMatrix();

	glDisable(GL_TEXTURE_2D);
}

void unicode_string_render(const char *text, float x, float y, float scale, unsigned int flags) {
	size_t array_size;
	vertex_t *vertex_array = create_vertex_array(text, &array_size);

	const float screen_scale = get_gl_height() / get_screen_height();

	render_vertex_array(vertex_array, array_size, x, y, screen_scale * scale, (gg_colour_t){ 1.0f, 0.0f, 0.0f, 1.0f });

	free(vertex_array);
}

void unicode_render_atlas(void) {
	glEnable(GL_TEXTURE_2D);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glBindTexture(GL_TEXTURE_2D, atlas->id);

	glPushMatrix();
	glBegin(GL_QUADS);

	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(0.0f, 0.0f, 1.0f);

	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(640.0f, 0.0f, 1.0f);

	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(640.0f, 480.0f, 1.0f);

	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(0.0f, 480.0f, 1.0f);

	glEnd();
	glPopMatrix();
	glDisable(GL_TEXTURE_2D);
}
