/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Xbox360_STFS.cpp: Microsoft Xbox 360 package reader.                    *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "Xbox360_STFS.hpp"
#include "librpbase/RomData_p.hpp"

#include "xbox360_stfs_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
using namespace LibRpBase;

// libi18n
#include "libi18n/i18n.h"

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>

namespace LibRomData {

ROMDATA_IMPL(Xbox360_STFS)

// Workaround for RP_D() expecting the no-underscore naming convention.
#define Xbox360_STFSPrivate Xbox360_STFS_Private

class Xbox360_STFS_Private : public RomDataPrivate
{
	public:
		Xbox360_STFS_Private(Xbox360_STFS *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(Xbox360_STFS_Private)

	public:
		// STFS type.
		enum StfsType {
			STFS_TYPE_UNKNOWN = -1,	// Unknown STFS type.

			STFS_TYPE_CON	= 0,	// Console-signed
			STFS_TYPE_PIRS	= 1,	// MS-signed for non-Xbox Live
			STFS_TYPE_LIVE	= 2,	// MS-signed for Xbox Live

			STFS_TYPE_MAX
		};
		int stfsType;

	public:
		// STFS headers.
		// NOTE: These are **NOT** byteswapped!
		STFS_Package_Header stfsHeader;
		STFS_Package_Metadata stfsMetadata;
};

/** Xbox360_STFS_Private **/

Xbox360_STFS_Private::Xbox360_STFS_Private(Xbox360_STFS *q, IRpFile *file)
	: super(q, file)
	, stfsType(STFS_TYPE_UNKNOWN)
{
	// Clear the headers.
	memset(&stfsHeader, 0, sizeof(stfsHeader));
	memset(&stfsMetadata, 0, sizeof(stfsMetadata));
}

/** Xbox360_STFS **/

/**
 * Read an Xbox 360 STFS file.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the disc image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open STFS file.
 */
Xbox360_STFS::Xbox360_STFS(IRpFile *file)
	: super(new Xbox360_STFS_Private(this, file))
{
	// This class handles application packages.
	// TODO: Change to Save File if the content is a save file.
	RP_D(Xbox360_STFS);
	d->className = "Xbox360_STFS";
	d->fileType = FTYPE_APPLICATION_PACKAGE;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the STFS header.
	d->file->rewind();
	size_t size = d->file->read(&d->stfsHeader, sizeof(d->stfsHeader));
	if (size != sizeof(d->stfsHeader)) {
		// Read error.
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Check if this file is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->stfsHeader);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->stfsHeader);
	info.ext = nullptr;	// Not needed for STFS.
	info.szFile = 0;	// Not needed for STFS.
	d->stfsType = isRomSupported_static(&info);
	d->isValid = (d->stfsType >= 0);

	if (!d->isValid) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Read the package metadata.
	// TODO: Only read on demand?
	size = d->file->read(&d->stfsMetadata, sizeof(d->stfsMetadata));
	if (size != sizeof(d->stfsMetadata)) {
		// Read error.
		d->file->unref();
		d->file = nullptr;
		return;
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int Xbox360_STFS::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(STFS_Package_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return Xbox360_STFS_Private::STFS_TYPE_UNKNOWN;
	}

	// Check for STFS.
	const STFS_Package_Header *const stfsHeader =
		reinterpret_cast<const STFS_Package_Header*>(info->header.pData);
	if (stfsHeader->magic == cpu_to_be32(STFS_MAGIC_CON)) {
		// We have a console-signed STFS package.
		return Xbox360_STFS_Private::STFS_TYPE_CON;
	} else if (stfsHeader->magic == cpu_to_be32(STFS_MAGIC_PIRS)) {
		// We have an MS-signed STFS package. (non-Xbox Live)
		return Xbox360_STFS_Private::STFS_TYPE_PIRS;
	} else if (stfsHeader->magic == cpu_to_be32(STFS_MAGIC_LIVE)) {
		// We have an MS-signed STFS package. (Xbox Live)
		return Xbox360_STFS_Private::STFS_TYPE_LIVE;
	}

	// Not supported.
	return Xbox360_STFS_Private::STFS_TYPE_UNKNOWN;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *Xbox360_STFS::systemName(unsigned int type) const
{
	RP_D(const Xbox360_STFS);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Xbox 360 has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"Xbox360_STFS::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	// TODO: STFS-specific, or just use Xbox 360?
	static const char *const sysNames[4] = {
		"Microsoft Xbox 360", "Xbox 360", "X360", nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions do not include the leading dot,
 * e.g. "bin" instead of ".bin".
 *
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *Xbox360_STFS::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		//".stfs",	// FIXME: Not actually used...

		nullptr
	};
	return exts;
}

/**
 * Get a list of all supported MIME types.
 * This is to be used for metadata extractors that
 * must indicate which MIME types they support.
 *
 * NOTE: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *Xbox360_STFS::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types.
		// TODO: Get these upstreamed on FreeDesktop.org.
		"application/x-xbox360-stfs",

		nullptr
	};
	return mimeTypes;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Xbox360_STFS::loadFieldData(void)
{
	RP_D(Xbox360_STFS);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->stfsType < 0) {
		// STFS file isn't valid.
		return -EIO;
	}

	// Parse the STFS file.
	// NOTE: The STFS headers are **NOT** byteswapped.
	const STFS_Package_Metadata *const stfsMetadata = &d->stfsMetadata;

	// Maximum of 14 fields, not including RomData subclasses.
	d->fields->reserve(14);
	d->fields->setTabName(0, "STFS");

	// Display name
	// TODO: Language ID?
	if (stfsMetadata->display_name[0][0] != 0) {
		d->fields->addField_string(C_("RomData", "Name"),
			utf16be_to_utf8(stfsMetadata->display_name[0],
				ARRAY_SIZE(stfsMetadata->display_name[0])));
	}

	// Description
	// TODO: Language ID?
	if (stfsMetadata->display_description[0][0] != 0) {
		d->fields->addField_string(C_("RomData", "Description"),
			utf16be_to_utf8(stfsMetadata->display_description[0],
				ARRAY_SIZE(stfsMetadata->display_description[0])));
	}

	// Publisher
	if (stfsMetadata->publisher_name[0] != 0) {
		d->fields->addField_string(C_("RomData", "Publisher"),
			utf16be_to_utf8(stfsMetadata->publisher_name,
				ARRAY_SIZE(stfsMetadata->publisher_name)));
	}

	// Title
	if (stfsMetadata->title_name[0] != 0) {
		d->fields->addField_string(C_("RomData", "Title"),
			utf16be_to_utf8(stfsMetadata->title_name,
				ARRAY_SIZE(stfsMetadata->title_name)));
	}

	// File type
	// TODO: Show console-specific information for 'CON '.
	static const char *const file_type_tbl[] = {
		NOP_C_("Xbox360_STFS|FileType", "Console-Specific Package"),
		NOP_C_("Xbox360_STFS|FileType", "Non-Xbox Live Package"),
		NOP_C_("Xbox360_STFS|FileType", "Xbox Live Package"),
	};
	if (d->stfsType > Xbox360_STFS_Private::STFS_TYPE_UNKNOWN &&
	    d->stfsType < Xbox360_STFS_Private::STFS_TYPE_MAX)
	{
		d->fields->addField_string(C_("Xbox360_STFS", "Package Type"),
			dpgettext_expr(RP_I18N_DOMAIN, "Xbox360_STFS|FileType",
				file_type_tbl[d->stfsType]));
	} else {
		d->fields->addField_string(C_("Xbox360_STFS|RomData", "Type"),
			C_("RomData", "Unknown"));
	}

	// Content type
	// TODO: Use binary search instead of switch/case?
	// TODO: Move to a data file.
	const char *s_content_type;
	switch (be32_to_cpu(stfsMetadata->content_type)) {
		case STFS_CONTENT_TYPE_SAVED_GAME:
			s_content_type = C_("Xbox360_STFS|ContentType", "Saved Game");
			break;
		case STFS_CONTENT_TYPE_MARKETPLACE_CONTENT:
			s_content_type = C_("Xbox360_STFS|ContentType", "Marketplace Content");
			break;
		case STFS_CONTENT_TYPE_PUBLISHER:
			s_content_type = C_("Xbox360_STFS|ContentType", "Publisher");
			break;
		case STFS_CONTENT_TYPE_XBOX_360_TITLE:
			s_content_type = C_("Xbox360_STFS|ContentType", "Xbox 360 Game");
			break;
		case STFS_CONTENT_TYPE_IPTV_PAUSE_BUFFER:
			s_content_type = C_("Xbox360_STFS|ContentType", "IPTV Pause Buffer");
			break;
		case STFS_CONTENT_TYPE_INSTALLED_GAME:
			s_content_type = C_("Xbox360_STFS|ContentType", "Installed Game");
			break;
		case STFS_CONTENT_TYPE_XBOX_ORIGINAL_GAME:
			s_content_type = C_("Xbox360_STFS|ContentType", "Original Xbox Game");
			break;
		case STFS_CONTENT_TYPE_AVATAR_ITEM:
			s_content_type = C_("Xbox360_STFS|ContentType", "Avatar Item");
			break;
		case STFS_CONTENT_TYPE_PROFILE:
			s_content_type = C_("Xbox360_STFS|ContentType", "Profile");
			break;
		case STFS_CONTENT_TYPE_GAMER_PICTURE:
			s_content_type = C_("Xbox360_STFS|ContentType", "Gamer Picture");
			break;
		case STFS_CONTENT_TYPE_THEME:
			s_content_type = C_("Xbox360_STFS|ContentType", "Theme");
			break;
		case STFS_CONTENT_TYPE_CACHE_FILE:
			s_content_type = C_("Xbox360_STFS|ContentType", "Cache File");
			break;
		case STFS_CONTENT_TYPE_STORAGE_DOWNLOAD:
			s_content_type = C_("Xbox360_STFS|ContentType", "Storage Download");
			break;
		case STFS_CONTENT_TYPE_XBOX_SAVED_GAME:
			// TODO: Verify.
			s_content_type = C_("Xbox360_STFS|ContentType", "Original Xbox Saved Game");
			break;
		case STFS_CONTENT_TYPE_XBOX_DOWNLOAD:
			// TODO: Verify.
			s_content_type = C_("Xbox360_STFS|ContentType", "Original Xbox Download");
			break;
		case STFS_CONTENT_TYPE_GAME_DEMO:
			s_content_type = C_("Xbox360_STFS|ContentType", "Game Demo");
			break;
		case STFS_CONTENT_TYPE_VIDEO:
			s_content_type = C_("Xbox360_STFS|ContentType", "Video");
			break;
		case STFS_CONTENT_TYPE_GAME_TITLE:
			s_content_type = C_("Xbox360_STFS|ContentType", "Game");
			break;
		case STFS_CONTENT_TYPE_INSTALLER:
			s_content_type = C_("Xbox360_STFS|ContentType", "Installer");
			break;
		case STFS_CONTENT_TYPE_GAME_TRAILER:
			s_content_type = C_("Xbox360_STFS|ContentType", "Game Trailer");
			break;
		case STFS_CONTENT_TYPE_ARCADE_TITLE:
			s_content_type = C_("Xbox360_STFS|ContentType", "Arcade Game");
			break;
		case STFS_CONTENT_TYPE_XNA:
			s_content_type = C_("Xbox360_STFS|ContentType", "XNA");
			break;
		case STFS_CONTENT_TYPE_LICENSE_STORE:
			s_content_type = C_("Xbox360_STFS|ContentType", "License Store");
			break;
		case STFS_CONTENT_TYPE_MOVIE:
			s_content_type = C_("Xbox360_STFS|ContentType", "Movie");
			break;
		case STFS_CONTENT_TYPE_TV:
			s_content_type = C_("Xbox360_STFS|ContentType", "TV");
			break;
		case STFS_CONTENT_TYPE_MUSIC_VIDEO:
			s_content_type = C_("Xbox360_STFS|ContentType", "Music Video");
			break;
		case STFS_CONTENT_TYPE_PODCAST_VIDEO:
			s_content_type = C_("Xbox360_STFS|ContentType", "Podcast Video");
			break;
		case STFS_CONTENT_TYPE_VIRAL_VIDEO:
			s_content_type = C_("Xbox360_STFS|ContentType", "Viral Video");
			break;
		case STFS_CONTENT_TYPE_GAME_VIDEO:
			s_content_type = C_("Xbox360_STFS|ContentType", "Game Video");
			break;
		case STFS_CONTENT_TYPE_COMMUNITY_GAME:
			s_content_type = C_("Xbox360_STFS|ContentType", "Community Game");
			break;
		default:
			s_content_type = nullptr;
			break;
	}

	if (s_content_type) {
		d->fields->addField_string(C_("Xbox360_STFS", "Content Type"), s_content_type);
	} else {
		d->fields->addField_string(C_("Xbox360_STFS", "Content Type"),
			rp_sprintf(C_("RomData", "Unknown (0x%08X)"),
				be32_to_cpu(stfsMetadata->content_type)));
	}

	// Media ID
	d->fields->addField_string(C_("Xbox360_STFS", "Media ID"),
		rp_sprintf("%08X", be32_to_cpu(stfsMetadata->media_id)),
		RomFields::STRF_MONOSPACE);

	// Title ID
	// TODO: Consolidate the Title ID formatting function?
	// FIXME: Verify behavior on big-endian.
	d->fields->addField_string(C_("Xbox360_XEX", "Title ID"),
		rp_sprintf_p(C_("Xbox360_XEX", "%1$08X (%2$c%3$c-%4$04u)"),
			be32_to_cpu(stfsMetadata->title_id.u32),
			stfsMetadata->title_id.a,
			stfsMetadata->title_id.b,
			be16_to_cpu(stfsMetadata->title_id.u16)),
		RomFields::STRF_MONOSPACE);

	// Version and base version
	// TODO: What indicates the update version?
	Xbox360_Version_t ver, basever;
	ver.u32 = be32_to_cpu(stfsMetadata->version.u32);
	basever.u32 = be32_to_cpu(stfsMetadata->base_version.u32);
	d->fields->addField_string(C_("Xbox360_XEX", "Version"),
		rp_sprintf("%u.%u.%u.%u",
			ver.major, ver.minor, ver.build, ver.qfe));
	d->fields->addField_string(C_("Xbox360_XEX", "Base Version"),
		rp_sprintf("%u.%u.%u.%u",
			basever.major, basever.minor, basever.build, basever.qfe));

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

}