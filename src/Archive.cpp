#include "Archive.hpp"

#include <QLibrary>
#include <QMimeDatabase>

#include <archive.h>
#include <archive_entry.h>

#include <memory>
#include <type_traits>
#include <utility>

#include "log.hpp"

template <auto fn>
using deleter_from_fn = std::integral_constant<decltype(fn), fn>;

template <typename T, auto fn>
using custom_unique_ptr = std::unique_ptr<T, deleter_from_fn<fn>>;

using archive_ptr = custom_unique_ptr<archive, archive_read_free>;

ReadArchiveWorker::ReadArchiveWorker(QString file_path, stop_token token) noexcept : file_path(std::move(file_path)), token(std::move(token))
{
}

void ReadArchiveWorker::run()
{
	QMimeDatabase mimedb;
	QVector<Entry> entries;

	// libarchive seems to be stream oriented and doesn't seem to support getting the entries or backtracking. So we will
	// open the file twice, once to get the list of entries and prepare the view, and a second time to extract the items.

	// Pass #1: Get the entries
	{
		auto archive = archive_ptr{archive_read_new()};
		auto err = archive_read_support_filter_all(archive.get());
		if (err != ARCHIVE_OK)
			emit error(tr("Error configuring archive filters: %1").arg(archive_error_string(archive.get())));

		err = archive_read_support_format_all(archive.get());
		if (err != ARCHIVE_OK)
			emit error(tr("Error configuring archive formats: %1").arg(archive_error_string(archive.get())));

		err = archive_read_support_compression_all(archive.get());
		if (err != ARCHIVE_OK)
			emit error(tr("Error configuring archive compression: %1").arg(archive_error_string(archive.get())));

		auto type = mimedb.mimeTypeForFile(file_path);
		LOG_DEBUG("Mime type '{0}' for file '{1}'", type.name().toStdString(), file_path.toStdString());

		if (token.stop_requested())
			return;

		// no idea what is a good size, so use 4k
		constexpr size_t block_size = 1024 * 4;
		err = archive_read_open_filename(archive.get(), file_path.toUtf8().data(), block_size);
		if (err != ARCHIVE_OK)
			emit error(tr("Error opening archive '%1'").arg(file_path));

		archive_entry *entry = nullptr;
		uint32_t index = 0;

		while (!token.stop_requested() && archive_read_next_header(archive.get(), &entry) == ARCHIVE_OK)
		{
			if (archive_entry_filetype(entry) == AE_IFDIR)
				continue;

			auto entry_name = archive_entry_pathname(entry);

			LOG_DEBUG("Got: {0}", entry_name);
			entries.push_back({index++, QString::fromUtf8(entry_name)});
		}

		if (token.stop_requested())
			return;

		LOG_DEBUG("# items in archive: {}", entries.size());
		emit contents(entries);
	}

	// Pass #2: extract the entries
	{
		auto archive = archive_ptr{archive_read_new()};
		auto err = archive_read_support_filter_all(archive.get());
		if (err != ARCHIVE_OK)
			emit error(tr("Error configuring archive filters: %1").arg(archive_error_string(archive.get())));

		err = archive_read_support_format_all(archive.get());
		if (err != ARCHIVE_OK)
			emit error(tr("Error configuring archive formats: %1").arg(archive_error_string(archive.get())));

		err = archive_read_support_compression_all(archive.get());
		if (err != ARCHIVE_OK)
			emit error(tr("Error configuring archive compression: %1").arg(archive_error_string(archive.get())));

		if (token.stop_requested())
			return;

		// no idea what is a good size, so use 4k
		constexpr size_t block_size = 1024 * 4;
		err = archive_read_open_filename(archive.get(), file_path.toUtf8().data(), block_size);
		if (err != ARCHIVE_OK)
			emit error(tr("Error opening archive '%1'").arg(file_path));

		LOG_DEBUG("Begin extracting files");
		archive_entry *entry = nullptr;
		uint32_t index = 0;
		while (!token.stop_requested() && archive_read_next_header(archive.get(), &entry) == ARCHIVE_OK)
		{
			if (archive_entry_filetype(entry) == AE_IFDIR)
				continue;

			auto &item = entries[index];
			LOG_DEBUG("Extracting #{0}: '{1}'", item.index, item.filename.toStdString());

			QByteArray content;
			const void *buf;
			size_t size;
			la_int64_t offset;

			while (archive_read_data_block(archive.get(), &buf, &size, &offset) == ARCHIVE_OK)
			{
				LOG_TRACE("reading, size: {0}, off: {1}", size, offset);
				if (content.size() != offset)
					LOG_WARN("Offset mismatch: expected {0}, got {1}", content.size(), offset);

				content.append(static_cast<const char *>(buf), int(size));
			}

			auto mimeType = mimedb.mimeTypeForFileNameAndData(item.filename, content);

			emit entryReady(item, {.type = mimeType, .content = std::move(content)});
			++index;
		}
		LOG_DEBUG("Finished extracting files");
	}
}
