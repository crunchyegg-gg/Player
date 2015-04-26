/*
 * This file is part of EasyRPG Player.
 *
 * EasyRPG Player is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EasyRPG Player is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with EasyRPG Player. If not, see <http://www.gnu.org/licenses/>.
 */

// Headers
#include "data.h"
#include "cache.h"
#include "output.h"
#include "utils.h"
#include "bitmap.h"
#include "font.h"
#include "text.h"
#include "game_system.h"

#include <cctype>

#include <boost/next_prior.hpp>
#include <boost/regex/pending/unicode_iterator.hpp>

void Text::Draw(Bitmap& dest, int x, int y, int color, std::string const& text, Text::Alignment align) {
	if (text.length() == 0) return;

	FontRef font = dest.GetFont();
	Rect dst_rect = font->GetSize(text);

	switch (align) {
	case Text::AlignCenter:
		dst_rect.x = x - dst_rect.width / 2; break;
	case Text::AlignRight:
		dst_rect.x = x - dst_rect.width; break;
	case Text::AlignLeft:
		dst_rect.x = x; break;
	default: assert(false);
	}

	dst_rect.y = y;
	dst_rect.width += 1; dst_rect.height += 1; // Need place for shadow
	if (dst_rect.IsOutOfBounds(dest.GetWidth(), dest.GetHeight())) return;

	BitmapRef text_surface; // Complete text will be on this surface
	text_surface = Bitmap::Create(dst_rect.width, dst_rect.height, true);
	text_surface->SetTransparentColor(dest.GetTransparentColor());
	text_surface->Clear();

	bool ready;
	BitmapRef system = Cache::System(ready);

	if (!ready) {
		return;
	}

	// Where to draw the next glyph (x pos)
	int next_glyph_pos = 0;

	// The current char is an exfont
	bool is_exfont = false;

	// This loops always renders a single char, color blends it and then puts
	// it onto the text_surface (including the drop shadow)
	for (boost::u8_to_u32_iterator<std::string::const_iterator>
			 c(text.begin(), text.begin(), text.end()),
			 end(text.end(), text.begin(), text.end()); c != end; ++c) {
		Rect next_glyph_rect(next_glyph_pos, 0, 0, 0);

		boost::u8_to_u32_iterator<std::string::const_iterator> next_c_it = boost::next(c);
		uint32_t const next_c = std::distance(c, end) > 1? *next_c_it : 0;

		// ExFont-Detection: Check for A-Z or a-z behind the $
		if (*c == '$' && std::isalpha(next_c)) {
			int exfont_value = -1;
			// Calculate which exfont shall be rendered
			if (islower(next_c)) {
				exfont_value = 26 + next_c - 'a';
			} else if (isupper(next_c)) {
				exfont_value = next_c - 'A';
			} else { assert(false); }
			is_exfont = true;

			Font::exfont->Render(*text_surface, next_glyph_rect.x, next_glyph_rect.y, *system, color, exfont_value);
		} else { // Not ExFont, draw normal text
			font->Render(*text_surface, next_glyph_rect.x, next_glyph_rect.y, *system, color, *c);
		}

		// If it's a full size glyph, add the size of a half-size glyph twice
		if (is_exfont) {
			is_exfont = false;
			next_glyph_pos += 12;
			// Skip the next character
			++c;
		} else {
			std::string const glyph(c.base(), next_c_it.base());
			next_glyph_pos += font->GetSize(glyph).width;
		}
	}

	BitmapRef text_bmp = Bitmap::Create(*text_surface, text_surface->GetRect());

	Rect src_rect(0, 0, dst_rect.width, dst_rect.height);
	int iy = dst_rect.y;
	if (dst_rect.height > text_bmp->GetHeight()) {
		iy += ((dst_rect.height - text_bmp->GetHeight()) / 2);
	}
	int ix = dst_rect.x;

	dest.Blit(ix, iy, *text_bmp, src_rect, 255);
}

void Text::Draw(Bitmap& dest, int x, int y, Color color, std::string const& text) {
	if (text.length() == 0) return;

	FontRef font = dest.GetFont();

	int next_glyph_pos = 0;

	for (boost::u8_to_u32_iterator<std::string::const_iterator>
			 c(text.begin(), text.begin(), text.end()),
			 end(text.end(), text.begin(), text.end()); c != end; ++c) {
		boost::u8_to_u32_iterator<std::string::const_iterator> next_c_it = boost::next(c);

		std::string const glyph(c.base(), next_c_it.base());
		if (*c == '\n') {
			y += font->GetSize(glyph).height;
			next_glyph_pos = 0;
			continue;
		}
		Rect next_glyph_rect(x + next_glyph_pos, y, 0, 0);

		font->Render(dest, next_glyph_rect.x, next_glyph_rect.y, color, *c);
		
		next_glyph_pos += font->GetSize(glyph).width;
	}
}
