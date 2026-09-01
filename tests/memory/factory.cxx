#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>

#include <warthog/memory/alloc/factory.h>
#include <warthog/memory/alloc/source_factory.h>
#include <warthog/memory/alloc/factory_pointer.h>
#include <warthog/memory/alloc/single_factory.h>
#include <warthog/memory/alloc/slab_factory.h>
#include <warthog/memory/alloc/block_factory.h>
#include <warthog/memory/alloc/bump_factory.h>
#include <warthog/memory/alloc/indexed_block_factory.h>
#include <warthog/memory/alloc/slice_array_factory.h>

#include <string_view>
#include <vector>
#include <set>
#include <unordered_set>

using namespace std::string_view_literals;

namespace warthog::memory::alloc {

template <typename T>
struct memblock {
	T a;
	T b;
};

TEST_CASE( "Basic factory check", "[factory]" ) {
	using memb = memblock<int32_t>;
	using memc = memblock<double>;

	SECTION( "concept check" ) {
		CHECK( ByteFactory<malloc_factory> );
		CHECK( ByteFactory<release_adaptor<malloc_factory>> );
		CHECK( SingleFactory<single_factory<malloc_factory, 4>> );
		CHECK( VoidFactory<void_factory> );
		CHECK_FALSE( VoidFactory<malloc_factory> );
		CHECK( ReleaseFactory<release_adaptor<malloc_factory>> );
		CHECK_FALSE( ReclaimFactory<release_adaptor<malloc_factory>> );
		CHECK( ByteFactory< factory_pointer< release_adaptor<malloc_factory>> > );
		CHECK( SingleFactory< factory_pointer<single_factory<malloc_factory, 4>> > );
	}

	SECTION( "single factory" ) {
		single_factory_type<malloc_factory, memb> factory;
		static_assert(SingleFactory<decltype(factory)>, "single_factory must be SingleFactory");
		REQUIRE( factory.element_size() == sizeof(memb) );
		REQUIRE( factory.alignment() >= alignof(memb) );
		auto p1 = reinterpret_cast<memb*>(factory.create());
		p1->a = 342;
		p1->b = 4224;
		auto p2 = reinterpret_cast<memb*>(factory.create());
		p2->a = 3242;
		factory.destroy(reinterpret_cast<std::byte*>(p2));
		auto p3 = reinterpret_cast<memb*>(factory.create());
		auto p4 = reinterpret_cast<memb*>(factory.create());
		p3->a = p4->b = 324222;
		factory.destroy(reinterpret_cast<std::byte*>(p4));
		factory.destroy(reinterpret_cast<std::byte*>(p1));
		factory.destroy(reinterpret_cast<std::byte*>(p3));
	}

	SECTION( "reuse factory" ) {
		reuse_adaptor<single_factory_type<malloc_factory, memb>> factory;
		static_assert(SingleFactory<decltype(factory)>, "factory must be SingleFactory");
		REQUIRE( factory.element_size() == sizeof(memb) );
		REQUIRE( factory.alignment() >= alignof(memb) );
		std::set<memb*> alloc;
		auto p1 = reinterpret_cast<memb*>(factory.create());
		alloc.insert(p1);
		p1->a = 342;
		p1->b = 4224;
		auto p2 = reinterpret_cast<memb*>(factory.create());
		alloc.insert(p2);
		p2->a = 3242;
		factory.destroy(reinterpret_cast<std::byte*>(p2));
		auto p3 = reinterpret_cast<memb*>(factory.create());
		CHECK( alloc.contains(p3) );
		auto p4 = reinterpret_cast<memb*>(factory.create());
		p3->a = p4->b = 324222;
		factory.destroy(reinterpret_cast<std::byte*>(p4));
		factory.destroy(reinterpret_cast<std::byte*>(p1));
		factory.destroy(reinterpret_cast<std::byte*>(p3));
	}

	SECTION( "area factory" ) {
		using base_bump_factory = bump_factory<slab_factory<malloc_factory, 0>>;
		static_assert(ArrayFactory<base_bump_factory>, "bump_factory must be ArrayFactory");
		base_bump_factory ba;
		slab_factory< factory_pointer<base_bump_factory>, 128, slab_memory_type<memb> > a1;
		slab_factory< factory_pointer<base_bump_factory>, 256, slab_memory_type<memc> > a2;
		static_assert(SlabFactory<decltype(a1)>, "must be SlabFactory");
		static_assert(SlabFactory<decltype(a2)>, "must be SlabFactory");

		const int SLAB_SIZE = GENERATE(1,16,128,1024) * 1024 + 32;
		REQUIRE( ba.setup(slab_factory_params(SLAB_SIZE)) );
		REQUIRE( a1.setup(ba) );
		REQUIRE( a2.setup(ba) );

		const int TOTAL = GENERATE(1,2,4,8,16,32) * 1024;
		for (int i = 0; i < TOTAL; ++i) {
			auto* p1 [[maybe_unused]] = a1.create();
			REQUIRE( p1 != nullptr );
			auto* p2 [[maybe_unused]] = a2.create();
			REQUIRE( p2 != nullptr );
		}
	}

	SECTION( "bump factory" ) {
		using bump_factory = bump_factory<slab_factory<malloc_factory, 256>>;
		static_assert(ArrayFactory<bump_factory>, "bump_factory must be ArrayFactory");
		single_factory_type<bump_factory, memb> ba;
		single_factory<bump_factory, 0, 0> bb;
		REQUIRE( ba.setup() );
		REQUIRE( ba.element_size() == sizeof(memb) );
		REQUIRE( ba.alignment() >= alignof(memb) );
		REQUIRE( bb.setup(dynamic_factory_params(sizeof(int32_t), alignof(int32_t))) );
		REQUIRE( bb.element_size() == sizeof(int32_t) );
		REQUIRE( bb.alignment() >= alignof(int32_t) );
		std::vector<memb*> ref;
		std::vector<int32_t*> refb;
		const int32_t TOTAL = GENERATE(1,2,4,8,16,32,64) * 1024;

		auto* big = reinterpret_cast<int32_t*>( ba.upstream().allocate(TOTAL * sizeof(int32_t)) );
		for (int32_t i = 0; i < TOTAL; ++i) {
			big[i] = -i;
		}
		for (int32_t i = 0; i < TOTAL; ++i) {
			auto* ptr = reinterpret_cast<memb*>( ba.create() );
			ptr->a = i;
			ptr->b = i*i;
			ref.push_back(ptr);
			int32_t* ptrb = reinterpret_cast<int32_t*>( bb.create() );
			*ptrb = i + i/2;
			refb.push_back(ptrb);
		}
		for (int32_t i = 0; i < TOTAL; ++i) {
			REQUIRE(big[i] == -i);
			auto* ptr = ref[i];
			REQUIRE(ptr->a == i);
			REQUIRE(ptr->b == i*i);
			auto* ptrb = refb[i];
			REQUIRE(*ptrb == i + i/2);
		}
	}

	SECTION( "block factory" ) {
		using area_fact = slab_factory<malloc_factory, 1024>;
		using block_fact_static = block_factory_type<memc, factory_pointer<area_fact>>;
		using block_fact_dyn = block_factory<factory_pointer<area_fact>>;
		static_assert(SingleFactory<block_fact_static>, "block_fact_static must be SingleFactory");
		static_assert(SingleFactory<block_fact_dyn>, "block_fact_dyn must be SingleFactory");
		area_fact pool;
		REQUIRE( pool.setup() );
		block_fact_static ba;
		REQUIRE( ba.setup(pool) );
		CHECK( ba.element_size() == sizeof(memc) );
		CHECK( ba.alignment() >= alignof(memc) );
		block_fact_dyn bb;
		REQUIRE( bb.setup(block_factory_params_type<memb>, pool) );
		CHECK( bb.element_size() == sizeof(memb) );
		CHECK( bb.alignment() >= alignof(memb) );
		std::vector<memc*> ref;
		std::vector<memb*> ref2;
		const int32_t TOTAL = GENERATE(1,2,4,8,16,32,64) * 1024;
		std::set<memc*> alloc;
		std::set<memb*> alloc2;

		for (int32_t i = 0; i < TOTAL; ++i) {
			memc* v = reinterpret_cast<memc*>(ba.create());
			v->a = i;
			v->b = i*i;
			REQUIRE_FALSE( alloc.contains(v) );
			alloc.insert(v);
			
			memb* v2 = reinterpret_cast<memb*>(bb.create());
			v2->a = i;
			v2->b = i*i;
			REQUIRE_FALSE( alloc2.contains(v2) );
			alloc2.insert(v2);
		}
		ba.reclaim();
		bb.reclaim();
		int32_t dup_count = 0;
		int32_t dup_count2 = 0;
		for (int32_t i = 0; i < TOTAL; ++i) {
			memc* v = reinterpret_cast<memc*>(ba.create());
			if (alloc.contains(v))
				dup_count += 1;
				
			memb* v2 = reinterpret_cast<memb*>(bb.create());
			if (alloc.contains(v))
				dup_count2 += 1;
		}
		// requires some reuse of blocks
		REQUIRE( dup_count > 0 );
		REQUIRE( dup_count2 > 0 );
	}

	SECTION( "buffer factory" ) {
		buffer_factory<1024> factory_void;
		static_assert(ByteFactory<decltype(factory_void)>, "factory_void must be ByteFactory");
		factory_void.setup();
		int valid = 0, invalid = 0;
		for (int i = 0; i < 16; i++) {
			auto* p = factory_void.allocate(128);
			if (p)
				valid += 1;
			else
				invalid += 1;
		}
		CHECK( valid != 0 );
		CHECK( valid <= 8 );
		CHECK( invalid >= 8 );
		factory_void.release();
		valid = 0, invalid = 0;
		for (int i = 0; i < 16; i++) {
			auto* p = factory_void.allocate(128);
			if (p)
				valid += 1;
			else
				invalid += 1;
		}
		CHECK( valid != 0 );
		CHECK( valid <= 8 );
		CHECK( invalid >= 8 );

		buffer_factory<1024, malloc_factory> factory_malloc;
		static_assert(ByteFactory<decltype(factory_malloc)>, "factory_malloc must be ByteFactory");
		factory_malloc.setup();
		valid = 0, invalid = 0;
		for (int i = 0; i < 64; i++) {
			auto* p = factory_malloc.allocate(128);
			if (p)
				valid += 1;
			else
				invalid += 1;
		}
		REQUIRE( invalid == 0 );
		factory_malloc.reclaim();
		valid = 0, invalid = 0;
		for (int i = 0; i < 64; i++) {
			auto* p = factory_malloc.allocate(128);
			if (p)
				valid += 1;
			else
				invalid += 1;
		}
		REQUIRE( invalid == 0 );

		using bfactory = bump_factory< slab_factory<malloc_factory, 4096> >;
		buffer_factory<1024, bfactory> factory_bump;
		static_assert(ByteFactory<decltype(factory_bump)>, "factory_malloc must be ByteFactory");
		factory_bump.setup();
		valid = 0, invalid = 0;
		for (int i = 0; i < 1024; i++) {
			auto* p = factory_bump.allocate(128);
			if (p)
				valid += 1;
			else
				invalid += 1;
		}
		REQUIRE( invalid == 0 );
		factory_bump.upstream().reclaim();
		factory_bump.release(false);
		valid = 0, invalid = 0;
		for (int i = 0; i < 1024; i++) {
			auto* p = factory_bump.allocate(128);
			if (p)
				valid += 1;
			else
				invalid += 1;
		}
		REQUIRE( invalid == 0 );
	}

	SECTION( "indexed block factory" ) {
		using area_fact = slab_factory<malloc_factory, 1024>;
		using indexed_fact = indexed_block_factory_type<memc, factory_pointer<area_fact>>;
		static_assert(SingleFactory<indexed_fact>, "block_factory must be SingleFactory");
		area_fact pool;
		REQUIRE( pool.setup() );
		indexed_fact ba;
		REQUIRE( ba.setup(pool) );
		CHECK( ba.element_size() == sizeof(memc) );
		CHECK( ba.alignment() >= alignof(memc) );

		std::vector<memc*> ref;
		const int32_t TOTAL = GENERATE(1,2,4,8,16,32,64) * 1024;
		std::set<memc*> alloc;

		// allocate inital 1024 elements one-at-a-time
		for (int32_t i = 0; i < TOTAL; ++i) {
			memc* v = reinterpret_cast<memc*>(ba.create());
			v->a = i;
			v->b = i*i;
			REQUIRE_FALSE( alloc.contains(v) );
			alloc.insert(v);
			ref.push_back(v);
		}
		REQUIRE( ba.size() == TOTAL );
		// check previous allocations
		for (int32_t i = 0; i < TOTAL; ++i) {
			memc* v = reinterpret_cast<memc*>(ba.get_if(i));
			REQUIRE( v == ref[i] );
			REQUIRE( v->a == i );
			REQUIRE( v->b == (i*i) );
		}
		const int32_t TOTAL2 = 10 * TOTAL;
		ba.resize(TOTAL2);
		for (int32_t i = 0; i < TOTAL; ++i) {
			memc* v = reinterpret_cast<memc*>(ba.get_if(i));
			REQUIRE( v == ref[i] );
		}
		for (int32_t i = TOTAL; i < TOTAL2; ++i) {
			memc* v = reinterpret_cast<memc*>(ba.get_if(i));
			v->a = i;
			v->b = i*i;
			REQUIRE_FALSE( alloc.contains(v) );
			alloc.insert(v);
			ref.push_back(v);
		}
	}

	SECTION( "runtime indexed block factory" ) {
		using area_fact = slab_factory<malloc_factory, 1024>;
		using indexed_fact = indexed_block_factory<factory_pointer<area_fact>, malloc_factory>;
		static_assert(SingleFactory<indexed_fact>, "block_factory must be SingleFactory");
		area_fact pool;
		REQUIRE( pool.setup() );
		indexed_fact ba;
		REQUIRE( ba.setup(indexed_block_factory_params_type<memc>, std::tuple<>(), pool) );
		CHECK( ba.element_size() == sizeof(memc) );
		CHECK( ba.alignment() >= alignof(memc) );

		std::vector<memc*> ref;
		const int32_t TOTAL = GENERATE(1,2,4,8,16,32,64) * 1024;
		std::set<memc*> alloc;

		// allocate inital 1024 elements one-at-a-time
		for (int32_t i = 0; i < TOTAL; ++i) {
			memc* v = reinterpret_cast<memc*>(ba.create());
			v->a = i;
			v->b = i*i;
			REQUIRE_FALSE( alloc.contains(v) );
			alloc.insert(v);
			ref.push_back(v);
		}
		REQUIRE( ba.size() == TOTAL );
		// check previous allocations
		for (int32_t i = 0; i < TOTAL; ++i) {
			memc* v = reinterpret_cast<memc*>(ba.get_if(i));
			REQUIRE( v == ref[i] );
			REQUIRE( v->a == i );
			REQUIRE( v->b == (i*i) );
		}
		const int32_t TOTAL2 = 10 * TOTAL;
		ba.resize(TOTAL2);
		for (int32_t i = 0; i < TOTAL; ++i) {
			memc* v = reinterpret_cast<memc*>(ba.get_if(i));
			REQUIRE( v == ref[i] );
		}
		for (int32_t i = TOTAL; i < TOTAL2; ++i) {
			memc* v = reinterpret_cast<memc*>(ba.get_if(i));
			v->a = i;
			v->b = i*i;
			REQUIRE_FALSE( alloc.contains(v) );
			alloc.insert(v);
			ref.push_back(v);
		}
	}

	SECTION( "slice array check" ) {
		using area_fact_base = bump_factory<slab_factory<malloc_factory, 1024*1024>>;
		using area_fact = slab_factory<area_fact_base, 1040>;

		using slice_fact = slice_array_factory_type<memb, factory_pointer<area_fact>, void_factory, 6, 0>;
		using slice_fact_d = slice_array_factory<factory_pointer<area_fact>, void_factory, 6, 2>;
		static_assert(ArrayFactory<slice_fact>, "slice_array_factory_type must be ArrayFactory");
		static_assert(ArrayFactory<slice_fact_d>, "slice_array_factory_type must be ArrayFactory");
		area_fact pool;
		REQUIRE( pool.setup() );
		slice_fact ba;
		REQUIRE( ba.setup(pool) );
		CHECK( ba.element_size() == sizeof(memb) );
		CHECK( ba.alignment() >= alignof(memb) );
		slice_fact_d bb;
		REQUIRE( bb.setup(slice_array_factory_params_type<memc>, pool) );
		CHECK( bb.element_size() == sizeof(memc) );
		CHECK( bb.alignment() >= alignof(memc) );

		const uint32_t TOTAL_R = GENERATE(1,2,4,8,16,32,64,100,200) * 1024;
		std::vector<std::pair<void*,int>> val;
		val.reserve(TOTAL_R+12);
		std::unordered_set<void*> ref;
		std::unordered_set<void*> ref_d;
		for (uint32_t i = 0; i < TOTAL_R; ++i)
		{
			int s = i % 6;
			int s1 = (1 << s) - GENERATE(take(1, random(0, 1<<6))) % (1 << (s > 1 ? s-1 : 0));
			auto p = ba.allocate_array(s1);
			REQUIRE( p.first != nullptr );
			REQUIRE( !ref.contains(p.first) );
			REQUIRE( p.second >= s1 );
			REQUIRE( p.second == 1 << s );
			val.push_back(p);
			ref.insert(p.first);
			
			auto p_d = bb.allocate_array(s1);
			REQUIRE( p_d.first != nullptr );
			REQUIRE( !ref_d.contains(p_d.first) );
			REQUIRE( p_d.second >= s1 );
			REQUIRE( p_d.second == std::max((int)(1 << s), (int)(1 << 2)) );
			ref_d.insert(p_d.first);
		}
		for (uint32_t i = 0; i < 16; ++i) {
			auto p = ba.allocate(GENERATE(take(1, random(1<<7, 1<<8))));
			REQUIRE(p != nullptr);
			auto p_d = bb.allocate(GENERATE(take(1, random(1<<7, 1<<8))));
			REQUIRE(p_d != nullptr);
		}
		{
			auto p = ba.allocate(1<<16);
			CHECK( p != nullptr );
			auto p_d = bb.allocate(1<<16);
			CHECK( p_d != nullptr );
		}

		// realloc
		std::unordered_set<void*> ref2;
		for (uint32_t i = 0; i < TOTAL_R/2; ++i)
		{
			ba.deallocate_array(reinterpret_cast<std::byte*>(val[i].first), val[i].second);
			ref2.insert(val[i].first);
		}
		bb.reclaim();
		// allocate, should reuse memory
		int count_c = 0;
		for (uint32_t i = 0; i < TOTAL_R/2; ++i) {
			int s = i % 6;
			int s1 = (1 << s) - GENERATE(take(1, random(0, 1<<6))) % (1 << (s > 1 ? s-1 : 0));
			auto p = ba.allocate(s1);
			CHECK( ref2.erase(p) == 1 );
			
			auto p_d = bb.allocate(s1);
			count_c += (int)ref_d.contains(p_d);
		}
		REQUIRE( count_c > 0 );
	}
}

}
