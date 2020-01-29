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
#include "freetype-gl/vector.h"

static texture_atlas_t *atlas;
static texture_font_t *font;
static unsigned int gpu_atlas_size;

typedef struct {
	float x, y, z;
	float s, t;
} vertex_t;

void check_error() {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        DBG_LOG("OpenGL error: %s\n", gluErrorString(err));
    }
}

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

unicode_string_t *unicode_string_create(const char *text) {
	unicode_string_t *string = vector_new(sizeof(vertex_t));

	size_t i;
	float pen_x = 0.0f;
	const float pen_y = 0.0f;
	for(i = 0; i < strlen(text); ++i) {
		texture_glyph_t *glyph = texture_font_get_glyph(font, text + i);

		if(glyph != NULL) {
			float kerning = 0.0f;
			if (i > 0)
				kerning = texture_glyph_get_kerning(glyph, text + i - 1);

			pen_x += kerning;

			int x0  = (int)(pen_x + glyph->offset_x);
			int y0  = (int)(pen_y + glyph->offset_y);
			int x1  = (int)(x0 + glyph->width);
			int y1  = (int)(y0 - glyph->height);

			float s0 = glyph->s0;
			float t0 = glyph->t0;
			float s1 = glyph->s1;
			float t1 = glyph->t1;

			vertex_t vertices[4] = {{x0, y0, 1.0f, s0, t0},
								   {x0, y1, 1.0f, s0, t1},
								   {x1, y1, 1.0f, s1, t1},
								   {x1, y0, 1.0f, s1, t0}};

			size_t j;
			for (j = 0; j < 4; ++j)
				vector_push_back(string, vertices + j);

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

	return string;
}

void unicode_string_render(unicode_string_t *string, float x, float y) {
	const float scale = get_gl_height() / get_screen_height();

	glEnable(GL_TEXTURE_2D);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glBindTexture(GL_TEXTURE_2D, atlas->id);

	glPushMatrix();
	glTranslatef(x, y, 0.0f);
	glScalef(scale, scale, 1.0f);
	glBegin(GL_QUADS);

	size_t i;
    for (i = 0; i < vector_size(string); ++i) {
		const vertex_t *vertex = vector_get(string, i);

		glTexCoord2f(vertex->s, vertex->t);
		glVertex3f(vertex->x, vertex->y, vertex->z);
	}

	glEnd();
	glPopMatrix();
	glDisable(GL_TEXTURE_2D);
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

void unicode_string_render_text(const char *text, float x, float y) {
	unicode_string_t *string = unicode_string_create(text);
	unicode_string_render(string, x, y);
	unicode_string_destroy(string);
}

void unicode_string_destroy(unicode_string_t *string) {
	vector_delete(string);
}
