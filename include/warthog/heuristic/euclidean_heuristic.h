#ifndef WARTHOG_HEURISTIC_EUCLIDEAN_HEURISTIC_H
#define WARTHOG_HEURISTIC_EUCLIDEAN_HEURISTIC_H

/// @file euclidean_heuristic.h
///
/// Analogue of Manhattan Heuristic but for 8C grids (cf. 4C).
///
/// @author Ryan Hechenberger
/// @date 2026-31-11

#include <warthog/constants.h>

#include "heuristic_value.h"
#include <warthog/domain/grid.h>
#include <warthog/util/helpers.h>

#include <concepts>
#include <tuple>
#include <span>
#include <cmath>

namespace warthog::heuristic
{

struct EuclideanXY
{
	constexpr auto operator()(const auto& c) const noexcept
	{
		return std::tuple(c.x, c.y);
	}
};

template <typename CoordArray = std::span<const grid::spoint32>, typename GetXY = EuclideanXY>
class euclidean_heuristic
{
public:
	constexpr euclidean_heuristic(auto&& crd) : coords_(std::move(crd))
	{ }

	~euclidean_heuristic() { }

	template <std::integral C>
	double
	h(C x, C y, C x2, C y2)
	{
		if constexpr (sizeof(C) <= 4) {
			int64_t dx = (int64_t)x - (int64_t)x2;
			dx = dx * dx;
			int64_t dy = (int64_t)x - (int64_t)x2;
			dy = dy * dy;
			return std::sqrt(static_cast<double>(dx) + static_cast<double>(dy));
		} else {
			return h(static_cast<double>(x), static_cast<double>(y), static_cast<double>(x2), static_cast<double>(y2));
		}
	}
	template <std::floating_point C>
	double
	h(C x, C y, C x2, C y2)
	{
		x -= x2;
		y -= y2;
		if constexpr (std::same_as<C, double>) {
			x *= x;
			y *= y;
			return std::sqrt(x + y);
		} else {
			double dx = static_cast<double>(x);
			dx *= dx;
			double dy = static_cast<double>(x);
			dy *= dy;
			return std::sqrt(dx + dy);
		}
	}

	double
	h(sn_id_t id, sn_id_t id2)
	{
		GetXY g_xy;
		auto [x, y] = g_xy(this->coords_[id]);
		auto [x2, y2] = g_xy(this->coords_[id2]);
		return this->h(x, y, x2, y2);
	}

	void
	h(heuristic_value* hv)
	{
		hv->lb_ = h(hv->from_, hv->to_);
	}

	constexpr size_t
	mem()
	{
		return sizeof(this);
	}

private:
	CoordArray coords_;
};

} // namespace warthog::heuristic

#endif // WARTHOG_HEURISTIC_OCTILE_HEURISTIC_H
