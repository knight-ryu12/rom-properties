/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * TImageTypesConfig.hpp: Image Types editor template.                     *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_CONFIG_TIMAGETYPESCONFIG_HPP__
#define __ROMPROPERTIES_LIBROMDATA_CONFIG_TIMAGETYPESCONFIG_HPP__

/**
 * NOTE: TImageTypesConfig.cpp MUST be #included by a file in
 * the UI frontend. Otherwise, the instantiated template won't
 * be compiled correctly.
 */

#include "../common.h"
#include "../RomData.hpp"

// C includes. (C++ namespace)
#include <cassert>

namespace LibRomData {

// NOTE: SysData_t is defined here because it causes
// problems if it's embedded inside of a templated class.
typedef uint32_t (*pFnSupportedImageTypes)(void);
struct SysData_t {
	const rp_char *name;			// System name.
	const char *classNameA;			// Class name in Config. (ASCII)
#if defined(_WIN32)
	const wchar_t *classNameW;		// Class name in Config. (Unicode)
#elif defined(RP_UTF16)
	const rp_char *classNameRP;		// Clas name in Config. (Unicode, rp_char)
#else /* defined(RP_UTF8) */
	#define classNameRP classNameA
#endif /* _WIN32 */
	pFnSupportedImageTypes getTypes;	// Get supported image types.
};
#ifdef _WIN32
#define SysDataEntry(klass, name) \
	{name, #klass, L#klass, LibRomData::klass::supportedImageTypes_static}
#elif defined(RP_UTF16)
#define SysDataEntry(klass, name) \
	{name, #klass, _RP(#klass), LibRomData::klass::supportedImageTypes_static}
#else /* defined(RP_UTF8) */
#define SysDataEntry(klass, name) \
	{name, #klass, LibRomData::klass::supportedImageTypes_static}
#endif

// MSVC 2010 doesn't support inline virtual functions.
// MSVC 2015 does support it.
// TODO: Check MSVC 2012 and 2013, and also gcc and clang.
#if !defined(_MSC_VER) || _MSC_VER >= 1700
#define INLINE_OVERRIDE inline
#else
#define INLINE_OVERRIDE
#endif

template<typename ComboBox>
class TImageTypesConfig
{
	public:
		TImageTypesConfig();
		virtual ~TImageTypesConfig();
	private:
		RP_DISABLE_COPY(TImageTypesConfig)

	public:
		// Number of image types. (columns)
		static const int IMG_TYPE_COUNT = RomData::IMG_EXT_MAX+1;
		// Number of systems. (rows)
		static const int SYS_COUNT = 8;

		/**
		 * (Re-)Load the configuration into the grid.
		 */
		void reset(void);

	public:
		// Image type data.
		static const rp_char *const imageTypeNames[IMG_TYPE_COUNT];

		// System data.
		static const SysData_t sysData[SYS_COUNT];

	public:
		/**
		 * Convert image type/system to ComboBox ID.
		 * @param sys System. (column)
		 * @param imageType Image type. (row)
		 * @return ComboBox ID.
		 */
		static inline unsigned int sysAndImageTypeToCbid(unsigned int sys, unsigned int imageType)
		{
			assert(sys < SYS_COUNT);
			assert(imageType < IMG_TYPE_COUNT);
			return (sys << 3) | imageType;
		}

		/**
		 * Get the image type from a ComboBox ID.
		 * @param cbid ComboBox ID.
		 * @return Image type. (column)
		 */
		static inline unsigned int imageTypeFromCbid(unsigned int cbid)
		{
			assert(cbid < (SYS_COUNT << 3));
			assert((cbid & 7) < IMG_TYPE_COUNT);
			return (cbid & 7);
		}

		/**
		 * Get the system from a ComboBox ID.
		 * @param cbid ComboBox ID.
		 * @return System. (row)
		 */
		static inline unsigned int sysFromCbid(unsigned int cbid)
		{
			assert(cbid < (SYS_COUNT << 3));
			assert((cbid & 7) < IMG_TYPE_COUNT);
			return (cbid >> 3);
		}

	public:
		/** UI signals. **/

		/**
		 * A ComboBox index was changed by the user.
		 * @param cbid ComboBox ID.
		 * @param prio New priority value. (0xFF == no)
		 */
		void cboImageType_priorityValueChanged(unsigned int cbid, unsigned int prio);

	public:
		/** Pure virtual functions. **/

		/**
		 * Set a ComboBox's current index.
		 * This will not trigger cboImageType_priorityValueChanged().
		 * @param cbid ComboBox ID.
		 * @param prio New priority value. (0xFF == no)
		 */
		INLINE_OVERRIDE virtual void cboImageType_setPriorityValue(unsigned int cbid, unsigned int prio) = 0;

	public:
		// Has the user changed anything?
		bool changed;

		// Combo box array.
		// NOTE: This is a square array, but no system supports
		// *all* image types, so most of these will be nullptr.
		// NOTE: Elements can be checked for nullptr, but the
		// pure virtual functions must be used to check the
		// actual contents.
		ComboBox cboImageType[SYS_COUNT][IMG_TYPE_COUNT];

		// Image types. (0xFF == No; others == priority)
		// NOTE: We need to store them here in order to handle
		// duplicate prevention, since ComboBox signals usually
		// don't include the "previous" index.
		uint8_t imageTypes[SYS_COUNT][IMG_TYPE_COUNT];

		// Number of valid image types per system.
		uint8_t validImageTypes[SYS_COUNT];

		// Which systems have the default configuration?
		// These ones will be saved with a blank value.
		bool sysIsDefault[SYS_COUNT];
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_CONFIG_TIMAGETYPESCONFIG_HPP__ */
