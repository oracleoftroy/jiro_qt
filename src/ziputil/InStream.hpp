#pragma once

#include <7zip/IStream.h>
#include <QFile>
#include <QString>

namespace ziputil
{
	// A QFile based IInStream implementation
	// NOTE: This implementation isn't reference counted and
	// expects to outlive the IInArchive opened with it
	class InStream final : public IInStream
	{
	public:
		explicit InStream(const QString &path);

	public: // IUnknown
		HRESULT QueryInterface(const IID &iid, void **p) override;
		ULONG AddRef() override;
		ULONG Release() override;

	public: // ISequentialInStream
		HRESULT Read(void *data, UInt32 size, UInt32 *processedSize) noexcept override;

	public: // IInStream
		HRESULT Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition) noexcept override;

	private:
		QFile file;
	};
}
