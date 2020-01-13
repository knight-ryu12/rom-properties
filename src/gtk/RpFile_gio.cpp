/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpFile_gio.cpp: IRpFile implementation using GIO/GVfs.                  *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "RpFile_gio.hpp"

#include <glib.h>
#include <gio/gio.h>

// C++ STL classes.
using std::string;

class RpFileGioPrivate
{
	public:
		RpFileGioPrivate(const char *uri)
			: stream(nullptr), uri(uri) { }
		RpFileGioPrivate(const string &uri)
			: stream(nullptr), uri(uri) { }
		~RpFileGioPrivate();

	private:
		RP_DISABLE_COPY(RpFileGioPrivate)

	public:
		GFileInputStream *stream;	// File input stream.
		string uri;			// GVfs URI.

	public:
		/**
		 * Convert a GIO error code to POSIX.
		 * @param gioerr GIO error code.
		 * @return POSIX error code.
		 */
		static int gioerr_to_posix(gint gioerr);
};

/** RpFileGioPrivate **/

RpFileGioPrivate::~RpFileGioPrivate()
{
	if (stream) {
		g_object_unref(stream);
	}
}

/**
 * Convert a GIO error code to POSIX.
 * @param gioerr GIO error code.
 * @return POSIX error code.
 */
int RpFileGioPrivate::gioerr_to_posix(gint gioerr)
{
	int err;
	switch (gioerr) {
		case G_IO_ERROR_NOT_FOUND:
			err = ENOENT;
			break;
		case G_IO_ERROR_IS_DIRECTORY:
			err = EISDIR;
			break;
		default:
			err = EIO;
			break;
	}
	return err;
}

/** RpFileGio **/

/**
 * Open a file.
 * NOTE: Files are always opened as read-only in binary mode.
 * @param uri GVfs URI.
 */
RpFileGio::RpFileGio(const char *uri)
	: super()
	, d_ptr(new RpFileGioPrivate(uri))
{
	init();
}

/**
 * Open a file.
 * NOTE: Files are always opened as read-only in binary mode.
 * @param uri GVfs URI.
 */
RpFileGio::RpFileGio(const string &uri)
	: super()
	, d_ptr(new RpFileGioPrivate(uri))
{
	init();
}

/**
 * Common initialization function for RpFile's constructors.
 * Filename must be set in d->filename.
 */
void RpFileGio::init(void)
{
	RP_D(RpFileGio);
	GError *err = nullptr;

	// Open the file.
	GFile *const file = g_file_new_for_uri(d->uri.c_str());
	d->stream = g_file_read(file, nullptr, &err);
	g_object_unref(file);
	if (!d->stream || err != nullptr) {
		// An error occurred.
		// TODO: GLib to POSIX error code conversion.
		if (err) {
			m_lastError = d->gioerr_to_posix(err->code);
			g_error_free(err);
		} else {
			// No GError...
			m_lastError = EIO;
		}

		if (d->stream) {
			g_object_unref(d->stream);
			d->stream = nullptr;
		}
		return;
	}

	// File is open.
	// TODO: Transparent gzip decompression?
}

RpFileGio::~RpFileGio()
{
	delete d_ptr;
}

/**
 * Is the file open?
 * This usually only returns false if an error occurred.
 * @return True if the file is open; false if it isn't.
 */
bool RpFileGio::isOpen(void) const
{
	RP_D(const RpFileGio);
	return (d->stream != nullptr);
}

/**
 * Close the file.
 */
void RpFileGio::close(void)
{
	RP_D(RpFileGio);
	if (d->stream) {
		g_object_unref(d->stream);
		d->stream = nullptr;
	}
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t RpFileGio::read(void *ptr, size_t size)
{
	RP_D(RpFileGio);
	if (!d->stream) {
		m_lastError = EBADF;
		return 0;
	}

	GError *err = nullptr;
	gssize ret = g_input_stream_read(G_INPUT_STREAM(d->stream),
		ptr, size, nullptr, &err);
	if (ret < 0 || err != nullptr) {
		// An error occurred.
		if (err) {
			m_lastError = d->gioerr_to_posix(err->code);
			g_error_free(err);
		} else {
			// No GError...
			m_lastError = EIO;
		}
		g_error_free(err);
		ret = 0;
	}

	return ret;
}

/**
 * Write data to the file.
 * (NOTE: Not valid for RpMemFile; this will always return 0.)
 * @param ptr Input data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes written.
 */
size_t RpFileGio::write(const void *ptr, size_t size)
{
	// Not a valid operation for RpFileGio.
	RP_UNUSED(ptr);
	RP_UNUSED(size);
	m_lastError = EBADF;
	return 0;
}

/**
 * Set the file position.
 * @param pos File position.
 * @return 0 on success; -1 on error.
 */
int RpFileGio::seek(int64_t pos)
{
	RP_D(RpFileGio);
	if (!d->stream) {
		m_lastError = EBADF;
		return -1;
	}

	int ret = 0;
	GError *err = nullptr;
	gboolean bRet = g_seekable_seek(G_SEEKABLE(d->stream),
		pos, G_SEEK_SET, nullptr, &err);
	if (bRet < 0 || err != nullptr) {
		// An error occurred.
		if (err) {
			m_lastError = d->gioerr_to_posix(err->code);
			g_error_free(err);
		} else {
			// No GError...
			m_lastError = EIO;
		}
		g_error_free(err);
		ret = -1;
	}

	return ret;
}

/**
 * Get the file position.
 * @return File position, or -1 on error.
 */
int64_t RpFileGio::tell(void)
{
	RP_D(RpFileGio);
	if (!d->stream) {
		m_lastError = EBADF;
		return -1;
	}

	return g_seekable_tell(G_SEEKABLE(d->stream));
}

/**
 * Truncate the file.
 * @param size New size. (default is 0)
 * @return 0 on success; -1 on error.
 */
int RpFileGio::truncate(int64_t size)
{
	// Not supported.
	// TODO: Writable RpFileGio?
	RP_UNUSED(size);
	m_lastError = ENOTSUP;
	return -1;
}

/** File properties **/

/**
 * Get the file size.
 * @return File size, or negative on error.
 */
int64_t RpFileGio::size(void)
{
	RP_D(RpFileGio);
	if (!d->stream) {
		m_lastError = EBADF;
		return -1;
	}

	// TODO: Error checking?
	GError *err = nullptr;
	GFileInfo *fileInfo = g_file_input_stream_query_info(d->stream,
		G_FILE_ATTRIBUTE_STANDARD_SIZE, nullptr, &err);
	if (!fileInfo || err != nullptr) {
		// An error occurred.
		// TODO: GLib to POSIX error code conversion.
		if (err) {
			m_lastError = d->gioerr_to_posix(err->code);
			g_error_free(err);
		} else {
			// No GError...
			m_lastError = EIO;
		}

		if (fileInfo) {
			g_object_unref(fileInfo);
		}
		return -1;
	}

	// Get the file size.
	goffset fileSize = g_file_info_get_size(fileInfo);
	g_object_unref(fileInfo);
	return fileSize;
}

/**
 * Get the filename.
 * NOTE: For RpFileGio, this returns a GVfs URI.
 * @return Filename. (May be empty if the filename is not available.)
 */
string RpFileGio::filename(void) const
{
	RP_D(const RpFileGio);
	return d->uri;
}