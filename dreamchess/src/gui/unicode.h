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

#ifndef GUI_UNICODE_H
#define GUI_UNICODE_H

typedef struct vector_t unicode_string_t;
int unicode_init(void);
void unicode_exit(void);
unicode_string_t *unicode_string_create(const char *text);
void unicode_string_destroy(unicode_string_t *string);
void unicode_string_render(unicode_string_t *string, float x, float y);
void unicode_string_render_text(const char *text, float x, float y);
void unicode_render_atlas(void);

#endif
