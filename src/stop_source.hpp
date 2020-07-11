#pragma once

#include <atomic>
#include <memory>

class stop_source;

// use std::stop_token when available
class stop_token final
{
private:
	friend class stop_source;

	explicit stop_token(std::shared_ptr<std::atomic_bool> token) noexcept
		: token(std::move(token))
	{}

public:
	[[nodiscard]] bool stop_requested() const noexcept
	{
		return token->load(std::memory_order_relaxed);
	}

private:
	std::shared_ptr<std::atomic_bool> token;
};

// use std::stop_source when available
class stop_source final
{
public:
	stop_source()
		: token(std::make_shared<std::atomic_bool>())
	{}

	[[nodiscard]] stop_token get_token() const noexcept
	{
		return stop_token{token};
	}

	void request_stop() noexcept
	{
		token->store(true, std::memory_order_relaxed);
	}

private:
	std::shared_ptr<std::atomic_bool> token;
};
