#include "SequentialOutStream.hpp"

#include "../log.hpp"

namespace ziputil
{
	HRESULT SequentialOutStream::QueryInterface([[maybe_unused]]const IID &iid, [[maybe_unused]]void **p)
	{
		LOG_TRACE("QueryInterface() called");

		if (iid == IID_IUnknown)
		{
			*p = static_cast<IUnknown*>(this);
			return S_OK;
		}

		if (iid == IID_ISequentialOutStream)
		{
			*p = static_cast<ISequentialOutStream*>(this);
			return S_OK;
		}

		return E_NOINTERFACE;
	}

	ULONG SequentialOutStream::AddRef()
	{
		LOG_TRACE("AddRef() called");
		return 1;
	}

	ULONG SequentialOutStream::Release()
	{
		LOG_TRACE("Release() called");
		return 1;
	}

	HRESULT SequentialOutStream::Write([[maybe_unused]]const void *buffer, [[maybe_unused]]UInt32 size, [[maybe_unused]]UInt32 *processedSize) noexcept
	{
		LOG_TRACE("Write({0}) called", size);
		data.append(static_cast<const char*>(buffer), size);

		if (processedSize)
			*processedSize = size;

		return S_OK;
	}

	void SequentialOutStream::reserve(UInt64 size) noexcept
	{
		if (size > data.capacity())
			data.reserve(size);
	}

	QByteArray SequentialOutStream::transferData() noexcept
	{
		return std::exchange(data, QByteArray());
	}
}
