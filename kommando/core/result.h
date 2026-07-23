#pragma once
#include <utility>
#include <variant>

namespace kommando
{
	template <typename V, typename E> class Result
	{
	public:
		Result(V val) : _data(std::move(val)), _isOk(true)
		{
		}

		static auto MakeError(E err) -> Result
		{
			Result r;
			r._data = std::move(err);
			r._isOk = false;
			return r;
		}

		[[nodiscard]] auto IsOk() const -> bool
		{
			return _isOk;
		}
		explicit operator bool() const
		{
			return IsOk();
		}

		[[nodiscard]] auto Get() const -> const V&
		{
			return std::get<V>(_data);
		}
		auto Get() -> V&
		{
			return std::get<V>(_data);
		}

		[[nodiscard]] auto Err() const -> const E&
		{
			return std::get<E>(_data);
		}
		auto Err() -> E&
		{
			return std::get<E>(_data);
		}

	private:
		Result() = default;
		std::variant<V, E> _data;
		bool _isOk = false;
	};

	template <typename E> class Result<void, E>
	{
	public:
		Result() : _isOk(true)
		{
		}

		static auto MakeError(E err) -> Result
		{
			Result r;
			r._isOk = false;
			r._err = std::move(err);
			return r;
		}

		[[nodiscard]] auto IsOk() const -> bool
		{
			return _isOk;
		}
		explicit operator bool() const
		{
			return IsOk();
		}

		[[nodiscard]] auto Err() const -> const E&
		{
			return _err;
		}
		auto Err() -> E&
		{
			return _err;
		}

	private:
		bool _isOk = false;
		E _err{};
	};
}