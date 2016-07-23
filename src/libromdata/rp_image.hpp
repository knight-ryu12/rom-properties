/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * rp_image.hpp: Image class.                                              *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

// C includes.
#include <stdint.h>

// C includes. (C++ namespace)
#include <cstring>

// TODO: Make this implicitly shared.
// TODO: Make suer all classes use typedef super.

namespace LibRomData {

class rp_image_private;
class rp_image
{
	public:
		enum Format {
			FORMAT_NONE,		// No image.
			FORMAT_CI8,		// Color index, 8-bit palette.
			FORMAT_ARGB32,		// 32-bit ARGB.
		};

		rp_image(int width, int height, Format format);
		~rp_image();

	private:
		friend class rp_image_private;
		rp_image_private *const d;
	private:
		rp_image(const rp_image &);
		rp_image &operator=(const rp_image &);

	public:
		/**
		 * Is the image valid?
		 * @return True if the image is valid.
		 */
		bool isValid(void) const;

		/**
		 * Get the image width.
		 * @return Image width.
		 */
		int width(void) const;

		/**
		 * Get the image height.
		 * @return Image height.
		 */
		int height(void) const;

		/**
		 * Get the image format.
		 * @return Image format.
		 */
		Format format(void) const;

		/**
		 * Get a pointer to the first line of image data.
		 * @return Image data.
		 */
		const void *bits(void) const;

		/**
		 * Get a pointer to the first line of image data.
		 * TODO: detach()
		 * @return Image data.
		 */
		void *bits(void);

		/**
		 * Get a pointer to the specified line of image data.
		 * @param i Line number.
		 * @return Line of image data, or nullptr if i is out of range.
		 */
		const void *scanLine(int i) const;

		/**
		 * Get a pointer to the specified line of image data.
		 * TODO: Detach.
		 * @param i Line number.
		 * @return Line of image data, or nullptr if i is out of range.
		 */
		void *scanLine(int i);

		/**
		 * Get the image data size, in bytes.
		 * This is width * height * pixel size.
		 * @return Image data size, in bytes.
		 */
		size_t data_len(void) const;

		/**
		 * Get the image palette.
		 * @return Pointer to image palette, or nullptr if not a paletted image.
		 */
		const uint32_t *palette(void) const;

		/**
		 * Get the image palette.
		 * @return Pointer to image palette, or nullptr if not a paletted image.
		 */
		uint32_t *palette(void);

		/**
		 * Get the number of elements in the image palette.
		 * @return Number of elements in the image palette, or 0 if not a paletted image.
		 */
		int palette_len(void) const;
};

}
