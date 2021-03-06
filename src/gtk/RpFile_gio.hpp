/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * RpFile_gio.hpp: IRpFile implementation using GIO/GVfs.                  *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_RPFILE_GIO_HPP__
#define __ROMPROPERTIES_GTK_RPFILE_GIO_HPP__

// librpfile
#include "librpfile/IRpFile.hpp"

class RpFileGioPrivate;
class RpFileGio : public LibRpFile::IRpFile
{
	public:
		/**
		 * Open a file.
		 * NOTE: Files are always opened as read-only in binary mode.
		 * @param uri GVfs URI.
		 */
		RpFileGio(const char *uri);
		RpFileGio(const std::string &uri);
	private:
		void init(void);
	protected:
		virtual ~RpFileGio();	// call unref() instead

	private:
		typedef LibRpFile::IRpFile super;
		RP_DISABLE_COPY(RpFileGio)
	protected:
		friend class RpFileGioPrivate;
		RpFileGioPrivate *const d_ptr;

	public:
		/**
		 * Is the file open?
		 * This usually only returns false if an error occurred.
		 * @return True if the file is open; false if it isn't.
		 */
		bool isOpen(void) const final;

		/**
		 * Close the file.
		 */
		void close(void) final;

		/**
		 * Read data from the file.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		size_t read(void *ptr, size_t size) final;

		/**
		 * Write data to the file.
		 * (NOTE: Not valid for RpFileGio; this will always return 0.)
		 * @param ptr Input data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes written.
		 */
		size_t write(const void *ptr, size_t size) final;

		/**
		 * Set the file position.
		 * @param pos File position.
		 * @return 0 on success; -1 on error.
		 */
		int seek(off64_t pos) final;

		/**
		 * Get the file position.
		 * @return File position, or -1 on error.
		 */
		off64_t tell(void) final;

		/**
		 * Truncate the file.
		 * (NOTE: Not valid for RpFileGio; this will always return -1.)
		 * @param size New size. (default is 0)
		 * @return 0 on success; -1 on error.
		 */
		int truncate(off64_t size = 0) final;

	public:
		/** File properties **/

		/**
		 * Get the file size.
		 * @return File size, or negative on error.
		 */
		off64_t size(void) final;

		/**
		 * Get the filename.
		 * NOTE: For RpFileGio, this returns a GVfs URI.
		 * @return Filename. (May be empty if the filename is not available.)
		 */
		std::string filename(void) const final;
};

#endif /* __ROMPROPERTIES_GTK_RPFILE_GIO_HPP__ */
