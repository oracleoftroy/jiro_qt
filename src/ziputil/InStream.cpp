#include "InStream.hpp"
#include "../log.hpp"

namespace ziputil
{
	InStream::InStream(const QString &path) : file(path)
	{
		if (!file.open(QIODevice::ReadOnly))
			LOG_WARN("Error opening file: '{0}'", path.toStdString());
	}

	HRESULT InStream::QueryInterface(const IID &iid, void **p)
	{
		LOG_TRACE("QueryInterface() called");

		if (iid == IID_IUnknown)
		{
			*p = static_cast<IUnknown *>(this);
			return S_OK;
		}

		if (iid == IID_ISequentialInStream)
		{
			*p = static_cast<ISequentialInStream *>(this);
			return S_OK;
		}

		if (iid == IID_IInStream)
		{
			*p = static_cast<IInStream *>(this);
			return S_OK;
		}

		return E_NOINTERFACE;
	}

	// We just stack allocate the file, so skip the reference counting
	ULONG InStream::AddRef()
	{
		LOG_TRACE("AddRef() called");
		return 1;
	}

	ULONG InStream::Release()
	{
		LOG_TRACE("Release() called");
		return 1;
	}

	HRESULT InStream::Read(void *data, UInt32 size, UInt32 *processedSize) noexcept
	{
		LOG_TRACE("Read() called requesting {0} bytes", size);
		if (file.error() != QFileDevice::NoError)
			return E_FAIL;

		UInt32 bytesRead = 0;
		int retry = 10;

		do
		{
			bytesRead = file.read(static_cast<char *>(data), size);
		} while (bytesRead == 0 && !file.atEnd() && retry--);

		if (processedSize)
			*processedSize = bytesRead;

		if (bytesRead == 0)
		{
			if (file.atEnd())
				LOG_TRACE("EOF reached, read {0} bytes", bytesRead);
			else if (retry < 0)
				LOG_TRACE("hit max number of retries");

			return S_FALSE;
		}

		LOG_TRACE("Read() successful, read {0} bytes", bytesRead);
		return S_OK;
	}

	HRESULT InStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition) noexcept
	{
		LOG_TRACE("Seek() called for offset {0} from {1}", offset, seekOrigin);

		if (file.error() != QFileDevice::NoError)
			return E_FAIL;

		qint64 from;
		switch (seekOrigin)
		{
		default:
			return E_INVALIDARG;
		case STREAM_SEEK_SET:
			from = 0;
			break;
		case STREAM_SEEK_CUR:
			from = file.pos();
			break;
		case STREAM_SEEK_END:
			from = file.size();
			break;
		}

		if (!file.seek(from + offset))
			return STG_E_INVALIDFUNCTION;

		if (newPosition)
			*newPosition = file.pos();

		LOG_TRACE("Seek() successful now at: {0}", file.pos());

		return S_OK;
	}
}
