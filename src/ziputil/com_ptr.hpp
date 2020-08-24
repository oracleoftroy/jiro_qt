#pragma once

#include <memory>
#include <type_traits>

namespace ziputil
{
	template <auto fn>
	using deleter_from_fn = std::integral_constant<decltype(fn), fn>;

	template <typename T, auto fn>
	using custom_unique_ptr = std::unique_ptr<T, deleter_from_fn<fn>>;

	template <typename T>
	concept ComObject = requires(T *x)
	{
		x->Release();
	};

	template <ComObject T>
	constexpr void com_release(T *p)
	{
		if (p)
			p->Release();
	}

	template <ComObject T>
	using com_ptr = custom_unique_ptr<T, com_release<T>>;
}
