#include "ArchiveExtractCallback.hpp"

#include "../log.hpp"
#include <7zip/IProgress.h>


#include <7zip/ICoder.h>

namespace ziputil
{
	HRESULT ArchiveExtractCallback::QueryInterface([[maybe_unused]] const IID &iid, [[maybe_unused]] void **p)
	{
		LOG_TRACE("QueryInterface() called");

		if (iid == IID_IUnknown)
		{
			LOG_TRACE("wanted IID_IUnknown");
			*p = static_cast<IUnknown *>(this);
			return S_OK;
		}

		if (iid == IID_IProgress)
		{
			LOG_TRACE("wanted IID_IProgress");
			*p = static_cast<IProgress *>(this);
			return S_OK;
		}

		if (iid == IID_IArchiveExtractCallback)
		{
			LOG_TRACE("wanted IID_IArchiveExtractCallback");
			*p = static_cast<IArchiveExtractCallback *>(this);
			return S_OK;
		}

		// We probably want to implement this
		if (iid == IID_ICryptoGetTextPassword)
			LOG_TRACE("wanted IID_ICryptoGetTextPassword, ignored");
		else if (iid == IID_ICompressProgressInfo)
			LOG_TRACE("wanted IID_ICompressProgressInfo, ignored");
		else if (iid == IID_IArchiveExtractCallbackMessage)
			LOG_TRACE("wanted IID_IArchiveExtractCallbackMessage, ignored");
		else
			LOG_TRACE("Unknown IID: {{{0:08x}-{1:04x}-{2:04x}-{3:02x}{4:02x}-{5:02x}{6:02x}{7:02x}{8:02x}{9:02x}{10:02x}}}", iid.Data1, iid.Data2, iid.Data3, iid.Data4[0], iid.Data4[1], iid.Data4[2], iid.Data4[3], iid.Data4[4], iid.Data4[5], iid.Data4[6], iid.Data4[7]);

		return E_NOINTERFACE;
	}

	ULONG ArchiveExtractCallback::AddRef()
	{
		LOG_TRACE("AddRef() called");
		return 1;
	}

	ULONG ArchiveExtractCallback::Release()
	{
		LOG_TRACE("Release() called");
		return 1;
	}

	// tells us the total number of bytes we will extract
	HRESULT ArchiveExtractCallback::SetTotal(UInt64 total) noexcept
	{
		LOG_TRACE("SetTotal({0}) called", total);
		out.reserve(total);
		return S_OK;
	}

	// tells us the total number of bytes we have extracted so far
	HRESULT ArchiveExtractCallback::SetCompleted(const UInt64 *completeValue) noexcept
	{
		if (completeValue)
			LOG_TRACE("SetCompleted(p -> {0}) called", *completeValue);
		else
			LOG_TRACE("SetCompleted(nullptr) called");

		return S_OK;
	}

	// 7-Zip doesn't call IArchiveExtractCallback functions
	//   GetStream()
	//   PrepareOperation()
	//   SetOperationResult()
	// from different threads simultaneously.
	// But 7-Zip can call functions for IProgress or ICompressProgressInfo functions
	// from another threads simultaneously with calls for IArchiveExtractCallback interface.

	// IArchiveExtractCallback::GetStream()
	//   UInt32 index - index of item in Archive
	//   Int32 askExtractMode  (Extract::NAskMode)
	//     if (askMode != NExtract::NAskMode::kExtract)
	//     {
	//       then the callee can not real stream: (*inStream == NULL)
	//     }

	//   Out:
	//       (*inStream == NULL) - for directories
	//       (*inStream == NULL) - if link (hard link or symbolic link) was created
	//       if (*inStream == NULL && askMode == NExtract::NAskMode::kExtract)
	//       {
	//         then the caller must skip extracting of that file.
	//       }

	//   returns:
	//     S_OK     : OK
	//     S_FALSE  : data error (for decoders)
	HRESULT ArchiveExtractCallback::GetStream([[maybe_unused]] UInt32 index, ISequentialOutStream **outStream, Int32 askExtractMode) noexcept
	{
		LOG_TRACE("GetStream({0}, outStr, {1}) called", index, askExtractMode);
		if (askExtractMode == NArchive::NExtract::NAskMode::kExtract)
			*outStream = &out;
		else
			*outStream = nullptr;

		return S_OK;
	}

	// pre-extract?
	HRESULT ArchiveExtractCallback::PrepareOperation([[maybe_unused]] Int32 askExtractMode) noexcept
	{
		LOG_TRACE("PrepareOperation({0}) called", askExtractMode);
		return S_OK;
	}

	// SetOperationResult()
	//   7-Zip calls SetOperationResult at the end of extracting,
	//   so the callee can close the file, set attributes, timestamps and security information.

	//   Int32 opRes (NExtract::NOperationResult)
	HRESULT ArchiveExtractCallback::SetOperationResult([[maybe_unused]] Int32 opRes) noexcept
	{
		auto to_str = [&] {
			const char *resultStr = "<unknown result code>";
			switch (opRes)
			{
			case NArchive::NExtract::NOperationResult::kOK:
				resultStr = "kOK";
				break;
			case NArchive::NExtract::NOperationResult::kUnsupportedMethod:
				resultStr = "kUnsupportedMethod";
				break;
			case NArchive::NExtract::NOperationResult::kDataError:
				resultStr = "kDataError";
				break;
			case NArchive::NExtract::NOperationResult::kCRCError:
				resultStr = "kCRCError";
				break;
			case NArchive::NExtract::NOperationResult::kUnavailable:
				resultStr = "kUnavailable";
				break;
			case NArchive::NExtract::NOperationResult::kUnexpectedEnd:
				resultStr = "kUnexpectedEnd";
				break;
			case NArchive::NExtract::NOperationResult::kDataAfterEnd:
				resultStr = "kDataAfterEnd";
				break;
			case NArchive::NExtract::NOperationResult::kIsNotArc:
				resultStr = "kIsNotArc";
				break;
			case NArchive::NExtract::NOperationResult::kHeadersError:
				resultStr = "kHeadersError";
				break;
			case NArchive::NExtract::NOperationResult::kWrongPassword:
				resultStr = "kWrongPassword";
				break;
			}
			return resultStr;
		};
		LOG_TRACE("SetOperationResult({0}) called", to_str());
		return S_OK;
	}

	QByteArray ArchiveExtractCallback::transferData() noexcept
	{
		return out.transferData();
	}
}
