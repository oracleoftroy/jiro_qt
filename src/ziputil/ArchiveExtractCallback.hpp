#pragma once

#include <7zip/Archive/IArchive.h>
#include <7zip/IPassword.h>

#include "SequentialOutStream.hpp"

namespace ziputil
{
	class ArchiveExtractCallback final : public IArchiveExtractCallback
	{
	public: // IUnknown
		HRESULT QueryInterface(const IID &iid, void **p) override;
		ULONG AddRef() override;
		ULONG Release() override;

	public: // IProgress
		HRESULT SetTotal(UInt64 total) noexcept override;
		HRESULT SetCompleted(const UInt64 *completeValue) noexcept override;

	public: // IArchiveExtractCallback
		HRESULT GetStream(UInt32 index, ISequentialOutStream **outStream, Int32 askExtractMode) noexcept override;
		HRESULT PrepareOperation(Int32 askExtractMode) noexcept override;
		HRESULT SetOperationResult(Int32 opRes) noexcept override;

	public: // Custom API
		QByteArray transferData() noexcept;

	private:
		SequentialOutStream out;
	};
}
