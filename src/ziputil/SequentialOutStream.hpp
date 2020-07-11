#pragma once

#include <QByteArray>

#include <7zip/IStream.h>

namespace ziputil
{
	class SequentialOutStream final : public ISequentialOutStream
	{
	public: // IUnknown
		HRESULT QueryInterface(const IID &iid, void **p) override;
		ULONG AddRef() override;
		ULONG Release() override;

	public: // ISequentialOutStream
		HRESULT Write(const void *buffer, UInt32 size, UInt32 *processedSize) noexcept override;

	public: // Custom API
		void reserve(UInt64 size) noexcept;
		QByteArray transferData() noexcept;

	private:
		QByteArray data;
	};
}
