#include "Archive.hpp"

#include <QLibrary>
#include <QMimeDatabase>

#include <utility>

#include "log.hpp"
#include "ziputil/ArchiveExtractCallback.hpp"
#include "ziputil/ArchiveFormats.hpp"
#include "ziputil/InStream.hpp"
#include "ziputil/SequentialOutStream.hpp"
#include "ziputil/com_ptr.hpp"

namespace
{
	static const GUID *chooseFormat(const QMimeType &type)
	{
		if (!type.isValid())
			return nullptr;

		if (type.inherits("application/zip"))
		{
			LOG_DEBUG("using zip format");
			return &Format_Zip;
		}

		if (type.inherits("application/x-7z-compressed"))
		{
			LOG_DEBUG("using 7z format");
			return &Format_SevenZip;
		}

		if (type.inherits("application/x-rar-compressed"))
		{
			LOG_DEBUG("using rar format");
			return &Format_Rar;
		}

		if (type.inherits("application/x-bzip2"))
		{
			LOG_DEBUG("using bzip2 format");

			return &Format_BZip2;
		}

		if (type.inherits("application/gzip"))
		{
			LOG_DEBUG("using gzip format");
			return &Format_GZip;
		}

		if (type.inherits("application/x-tar"))
		{
			LOG_DEBUG("using tar format");
			return &Format_Tar;
		}

		return nullptr;
	}
}

ReadArchiveWorker::ReadArchiveWorker(QString file_path, stop_token token) noexcept : file_path(std::move(file_path)), token(std::move(token))
{
}

void ReadArchiveWorker::run()
{
	QLibrary lib("7z");
	if (!lib.load())
	{
		emit error("Could not find 7z library");
		return;
	}

	auto CreateObject = reinterpret_cast<Func_CreateObject>(lib.resolve("CreateObject"));
	if (!CreateObject)
	{
		emit error("Could not find 'CreateInArchive()'");
		return;
	}

	if (token.stop_requested())
		return;

	QMimeDatabase mimedb;
	auto type = mimedb.mimeTypeForFile(file_path);
	LOG_INFO("Mime type '{0}' for file '{1}'", type.name().toStdString(), file_path.toStdString());

	auto format = chooseFormat(type);
	if (!format)
	{
		emit error(tr("Could not find decompressor for %1").arg(type.name()));
		return;
	}

	IInArchive *p = nullptr;

	if (CreateObject(format, &IID_IInArchive, reinterpret_cast<void **>(&p)) != S_OK)
	{
		LOG_ERROR("Could not create IInArchive for zip format");
		emit error("Could not create IInArchive for zip format");
		return;
	}

	if (token.stop_requested())
		return;

	ziputil::com_ptr<IInArchive> inArchive{p};
	ziputil::InStream inStream{file_path};

	if (auto hr = inArchive->Open(&inStream, nullptr, nullptr); hr != S_OK)
	{
		emit error("Error opening Archive");
		return;
	}

	UInt32 numItems;
	inArchive->GetNumberOfItems(&numItems);
	LOG_DEBUG("# items in archive: {0}", numItems);

	UInt32 numProps;
	inArchive->GetNumberOfProperties(&numProps);
	LOG_DEBUG("# props in archive: {0}", numProps);

	// STDMETHOD(GetPropertyInfo)(UInt32 index, BSTR *name, PROPID *propID, VARTYPE *varType) MY_NO_THROW_DECL_ONLY x; \
	// STDMETHOD(GetProperty)(UInt32 index, PROPID propID, PROPVARIANT *value) MY_NO_THROW_DECL_ONLY x; \

	if (token.stop_requested())
		return;

	QVector<Entry> entries;
	PROPVARIANT value{};

	auto isDirectory = [&](auto i) { return S_OK == inArchive->GetProperty(i, kpidIsDir, &value) && value.vt == VT_BOOL && value.boolVal == VARIANT_TRUE; };

	for (unsigned int i = 0; i < numItems && !token.stop_requested(); ++i)
	{
		if (isDirectory(i))
		{
			LOG_DEBUG("Skipping directory");
			continue;
		}

		inArchive->GetProperty(i, kpidPath, &value);

		if (value.vt == VT_BSTR)
			entries.push_back({i, QString::fromWCharArray(value.bstrVal, SysStringLen(value.bstrVal))});
		else
		{
			LOG_WARN("Could not read path for index {0}, type is {1}", i, value.vt);
			entries.push_back({i, "<unknown>"});
		}
	}

	if (token.stop_requested())
		return;
	emit contents(entries);

	ziputil::ArchiveExtractCallback callback;

	LOG_DEBUG("Begin extracting files");
	for (auto &entry : entries)
	{
		if (token.stop_requested())
			break;

		LOG_DEBUG("Extracting #{0}: '{1}'", entry.index, entry.filename.toStdString());

		if (auto hr = inArchive->Extract(&entry.index, 1, 0, &callback); hr != S_OK)
			LOG_WARN("Error {2:X} extracting #{0}: '{1}'", entry.index, entry.filename.toStdString(), hr);

		auto content = callback.transferData();
		auto mimeType = mimedb.mimeTypeForFileNameAndData(entry.filename, content);

		emit entryReady(entry, {.type = mimeType, .content = std::move(content)});
	}
}
