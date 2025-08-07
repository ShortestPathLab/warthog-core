#include <warthog/io/grid_trace.h>
#include <exception>
#include <format>

namespace warthog::io
{


void grid_trace::print_posthoc_header()
{
	if (*this) {
		stream() << R"posthoc(version: 1.4.0
views:
  cell:
    - $: rect
      width: 1
      height: 1
      x: ${{$.x}}
      y: ${{$.y}}
      fill: ${{$.fill}}
      clear: ${{ $.clear }}
  main:
    - $: cell
      $if: ${{ $.type == 'source' }}
      fill: green
      clear: false
    - $: cell
      $if: ${{ $.type == 'destination' }}
      fill: red
      clear: false
    - $: cell
      $if: ${{ $.type == 'expand' }}
      fill: blue
      clear: close
    - $: cell
      $if: ${{ $.type == 'generate' }}
      fill: orange
      clear: true
pivot:
  x: ${{ $.x + 0.5 }}
  y: ${{ $.y + 0.5 }}
  scale: 1
events:
)posthoc";
	}
}

void grid_trace::begin_search(int id, const search::search_problem_instance& pi)
{
	posthoc_listener::begin_search(id);
	if (*this) {
		assert(grid_ != nullptr);
		uint32_t x, y;
		grid_->to_unpadded_xy(pi.start_, x, y);
		stream() << std::format("  - {{ type: source, id: {}, x: {}, y: {} }}\n",
			pi.start_.id, x, y
		);
		grid_->to_unpadded_xy(pi.target_, x, y);
		stream() << std::format("  - {{ type: destination, id: {}, x: {}, y: {} }}\n",
			pi.target_.id, x, y
		);
	}
}

void
grid_trace::expand_node(const node& current) const
{
	if (*this) {
		assert(grid_ != nullptr);
		uint32_t x, y;
		grid_->to_unpadded_xy(current.get_id(), x, y);
		stream() << std::format("  - {{ type: expand, id: {}, x: {}, y: {}, f: {}, g: {} }}\n",
			current.get_id().id, x, y, current.get_f(), current.get_g()
		);
	}
}

void
grid_trace::relax_node(const node& current) const
{
	if (*this) {
		assert(grid_ != nullptr);
		uint32_t x, y;
		grid_->to_unpadded_xy(current.get_id(), x, y);
		stream() << std::format("  - {{ type: expand, id: {}, x: {}, y: {}, f: {}, g: {} }}\n",
			current.get_id().id, x, y, current.get_f(), current.get_g()
		);
	}
}


void
grid_trace::close_node(const node& current) const
{
	if (*this) {
		assert(grid_ != nullptr);
		uint32_t x, y;
		grid_->to_unpadded_xy(current.get_id(), x, y);
		stream() << std::format("  - {{ type: close, id: {}, x: {}, y: {}, f: {}, g: {} }}\n",
			current.get_id().id, x, y, current.get_f(), current.get_g()
		);
	}
}


void
grid_trace::generate_node(const node* parent, const node& child, cost_t, uint32_t) const
{
	if (*this) {
		assert(grid_ != nullptr);
		std::string pid;
		if (parent != nullptr) {
			pid = std::format(", pId: {}", parent->get_id().id);
		}
		uint32_t x, y;
		grid_->to_unpadded_xy(child.get_id(), x, y);
		stream() << std::format("  - {{ type: generate, id: {}{}, x: {}, y: {}, f: {}, g: {} }}\n",
			child.get_id().id, pid, x, y, child.get_f(), child.get_g()
		);
	}
}

} // namespace warthog::io
