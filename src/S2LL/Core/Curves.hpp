#pragma once

#include <array>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

namespace S2LL
{
	struct E2;
	struct E3;
	struct S2;
	struct LL;
	/// Variable/fixed-size storage selector for N=0 (vector) vs N>0 (array)
	template <typename V, size_t N>
	struct LoopStorage
	{
		using type = std::array<V, N>;
	};

	template <typename V>
	struct LoopStorage<V, 0>
	{
		using type = std::vector<V>;
	};

	/// Generic template for polyloop (closed polychain) without edge realization
	template <typename V, size_t N = 0>
	struct Loop
	{
		using value_type = V;
		using storage_type = typename LoopStorage<V, N>::type;

		/// List of vertices
		storage_type vertices;

		constexpr Loop() = default;

		constexpr Loop(storage_type v) : vertices(std::move(v)) {}

		constexpr Loop(std::initializer_list<V> list) requires (N == 0)
			: vertices(list) {}

		constexpr Loop(std::initializer_list<V> list) requires (N > 0)
		{
			size_t i = 0;
			for (const auto& elem : list)
			{
				if (i < N) vertices[i++] = elem;
			}
		}

		/// Cyclic index resolution:
		///   [0, n)  -> direct access (no division)
		///   [n, +Inf) -> i % n          (single mod, result already >= 0)
		///   (-Inf, 0) -> n - 1 - ((-i - 1) % n)  (single mod, avoids double-mod)
		constexpr const V& operator[](ptrdiff_t i) const noexcept
		{
			const ptrdiff_t n = static_cast<ptrdiff_t>(vertices.size());
			if (i >= 0 && i < n) return vertices[static_cast<size_t>(i)];
			if (i >= n)          return vertices[static_cast<size_t>(i % n)];
			/*  i < 0  */        return vertices[static_cast<size_t>(n - 1 - ((-i - 1) % n))];
		}

		constexpr V& operator[](ptrdiff_t i) noexcept
		{
			const ptrdiff_t n = static_cast<ptrdiff_t>(vertices.size());
			if (i >= 0 && i < n) return vertices[static_cast<size_t>(i)];
			if (i >= n)          return vertices[static_cast<size_t>(i % n)];
			/*  i < 0  */        return vertices[static_cast<size_t>(n - 1 - ((-i - 1) % n))];
		}

		constexpr size_t size() const noexcept { return vertices.size(); }

		// Implicit conversion to std::span<const V>
		constexpr operator std::span<const V>() const noexcept
		{
			return std::span<const V>(vertices.data(), vertices.size());
		}

		constexpr operator std::span<V>() noexcept
		{
			return std::span<V>(vertices.data(), vertices.size());
		}
	};

	/// Span-based, storage-incognizant views for algorithms
	template <typename V>
	using LoopView = std::span<const V>;

	using E2View = LoopView<E2>;
	using E3View = LoopView<E3>;
	using S2View = LoopView<S2>;
	using LLView = LoopView<LL>;
}
