#include <warthog/manager/grid_patch_set.h>

#include <warthog/io/grid.h>
#include <warthog/io/log.h>

#include <limits>

namespace warthog::manager
{

bool
grid_patch_set::load(std::istream& file, int max_grids)
{
	io::bittable_serialize S;
	if(auto ec = S.open_read(&file); ec != std::errc{})
	{
		WARTHOG_GWARN_FMT("grid patch failed to open file code={}", (int)ec);
		return false;
	}
	if(int r = deserialize(S, max_grids); r < 0)
	{
		WARTHOG_GWARN_FMT("grid patch failed to read patch {}", -r - 1);
		return false;
	}
	return true;
}

bool
grid_patch_set::load(const std::filesystem::path& maps, int max_grids)
{
	io::bittable_serialize S;
	S.set_filename(std::filesystem::path(maps));
	if(auto ec = S.open_read(); ec != std::errc{})
	{
		WARTHOG_GWARN_FMT("grid patch failed to open file code={}", (int)ec);
		return false;
	}
	if(int r = deserialize(S); r < 0)
	{
		WARTHOG_GWARN_FMT("grid patch failed to read patch {}", -r - 1);
		return false;
	}
	return true;
}

int
grid_patch_set::deserialize(
    io::bittable_serialize& S, int max_grids, uint32_t flags)
{
	// read header
	if(S.get_type() == io::bittable_type::NONE)
	{
		// header not read
		if((flags & SKIP_HEADER) != 0)
		{
			WARTHOG_GWARN("grid patch SKIP_HEADER for serializer when header "
			              "has not been read.");
			return -1;
		}
		if(auto ec = S.read_header(); ec != std::errc{})
		{
			WARTHOG_GWARN_FMT(
			    "grid patch failed to read header code={}", (int)ec);
			return -1;
		}
	}
	else
	{
		// header has been read, check
		if((flags & FORCE_HEADER) != 0)
		{
			WARTHOG_GWARN("grid patch FORCE_HEADER for serializer when header "
			              "has already been read.");
			return -1;
		}
	}
	auto type = S.get_type();
	if(type != io::bittable_type::OCTILE && type != io::bittable_type::PATCH)
	{
		WARTHOG_GWARN(
		    "grid patch only supports reading type octile or patch.");
		return -1;
	}

	// start reading grids
	int count = 0;
	if(max_grids < 0) { max_grids = std::numeric_limits<int>::max(); }
	while(count < max_grids && S.get_patch_count() < S.get_patch_amount())
	{
		// read new grid
		++count;
		if(auto ec = S.read_grid_header(); ec != std::errc{})
		{
			WARTHOG_GWARN_FMT(
			    "grid patch failed to read patch {} header code={}", count,
			    (int)ec);
			return -count;
		}
		if((flags & IGNORE_INDEX) == 0)
		{
			// check index
			if(S.get_patch_id() != patches_.size())
			{
				// index mismatch
				WARTHOG_GWARN_FMT(
				    "grid patch failed patch {} index mismatch of {} expected "
				    "{}",
				    count, S.get_patch_id(), patches_.size());
				return -count;
			}
		}
		// setup patch data
		auto dim        = S.get_dim();
		auto bytes      = bittable::calc_array_size(dim.width, dim.height);
		auto* grid_data = static_cast<bittable::value_type*>(
		    grid_res_.allocate(bytes, alignof(bittable::value_type)));
		bittable patch(grid_data, dim.width, dim.height);
		if(auto ec = S.read_grid_data(patch); ec != std::errc{})
		{
			WARTHOG_GWARN_FMT(
			    "grid patch failed to read patch {} data code={}", count,
			    (int)ec);
			return -count;
		}
		patches_.push_back(patch);
	}

	return count;
}

bool
grid_patch_set::push_copy(
    bittable table, uint16_t offset_x, uint16_t offset_y, uint16_t width,
    uint16_t height)
{
	if(table.size() == 0) { return false; }
	bittable patch;
	if(offset_x == 0 && offset_y == 0 && width == npos && height == npos)
	{
		// copy as is
		auto size       = table.size_bytes();
		auto* grid_data = static_cast<bittable::value_type*>(
		    grid_res_.allocate(size, alignof(bittable::value_type)));
		std::memcpy(grid_data, table.data(), size);
		patch = bittable(grid_data, table.width(), table.height());
	}
	else
	{
		// crop
		if(offset_x >= table.width() || width > table.width()
		   || offset_x + width > table.width())
		{
			return false;
		}
		if(offset_y >= table.height() || height > table.height()
		   || offset_y + height > table.height())
		{
			return false;
		}
		auto size       = bittable::calc_array_size(width, height);
		auto* grid_data = static_cast<bittable::value_type*>(
		    grid_res_.allocate(size, alignof(bittable::value_type)));
		patch = bittable(grid_data, table.width(), table.height());
		table.copy(
		    patch, pad_id::zero(), table.xy_to_id(offset_x, offset_y), width,
		    height);
	}
	patches_.push_back(patch);
	return true;
}

bool
grid_patch_set::push_ref(bittable patch)
{
	patches_.push_back(patch);
	return true;
}

void
grid_patch_set::reset()
{
	patches_.clear();
	grid_res_.release();
}

} // namespace warthog::manager
