#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>

#include <warthog/forward.h>
#include <warthog/util/fixed_point.h>

#include <cmath>

namespace
{

using namespace ::warthog::forward_types;

TEMPLATE_TEST_CASE_SIG("fixed_point tests", "[unit][template][fixedpoint]", ((int I, int F), I, F), (4,4), (6,0), (6,6), (10,6), (14,18), (32,0), (32,16), (32,32), (40,20))
{
	constexpr int D = I+F;
	using ::warthog::util::binary_fixed_point;
	using fp = binary_fixed_point<D,F>;

	SECTION("type check")
	{
		REQUIRE( fp::digits() == D );
		REQUIRE( fp::frac() == F );
		REQUIRE( fp::overflow() == false );
		REQUIRE( fp::frac_mask() == (int64_t)((1ull << F) - 1) );
		REQUIRE( ::warthog::util::BinaryFixedPoint<fp> );
	}

	SECTION("range checks")
	{
		auto v = GENERATE( 0x1.ffffffffp0L, -0x1.ffffffffp0L, 0x1.00000001p0L, -0x1.00000001p0L, take(1000, random(-(long double)((1ull << (I-1))-1), (long double)((1ull << (I-1))-1))) );
		long double vi;
		long double vf;
		vf = std::modf((long double)v, &vi);

		long double vceil = std::ceil(v);
		long double vfloor = std::floor(v);
		
		fp p(v);

		CHECK( int_trunc(p) == (int64_t)vi );
		CHECK( trunc(p) == fp(vi) );

		if (std::abs(vf) > std::ldexp(1.0L, -F)) {
			// not too small to truncate frac away
			CHECK( int_floor(p) == (int64_t)vfloor );
			CHECK( floor(p) == fp(vfloor) );
			CHECK( int_ceil(p) == (int64_t)vceil );
			CHECK( ceil(p) == fp(vceil) );
			CHECK( static_cast<int64_t>(p) == (int64_t)vfloor );
			
		}

		long double pf = static_cast<long double>(p);
		long double pfi;
		long double pff;
		pff = std::modf((long double)pf, &pfi);

		CHECK( std::abs(pff) <= std::abs(vf) );
		CHECK( pfi == vi );
		CHECK( std::abs(vf - pff) <= std::ldexp(1.0L, F) );
	}
}

TEST_CASE("fixed_point operator tests", "[unit][general][fixedpoint]")
{
	using ::warthog::util::binary_fixed_point;
	using fp1 = binary_fixed_point<16,8>;
	using fp2 = binary_fixed_point<32,10>;

#define WARTHOG_TEST_PDEF(var,ints,pow,num) constexpr int64_t n##var = num, x##var = pow, d##var = 1LL<<pow; \
		const binary_fixed_point<ints+pow,pow> f##var (n##var);
	WARTHOG_TEST_PDEF(1,8,2,37);
	WARTHOG_TEST_PDEF(2,8,2,-26474);
	WARTHOG_TEST_PDEF(3,22,10,0x0003'66ffLL);
	WARTHOG_TEST_PDEF(4,22,10,0x5fff'ffffLL);

	auto p12 = f1 + f2;
	CHECK( p12.digits() == 10 );
	CHECK( p12.digits() == p12.logical_digits() );
	CHECK( p12.frac() == 2 );
	auto n12 = f1 - f2;
	CHECK( n12.digits() == 10 );
	CHECK( n12.digits() == n12.logical_digits() );
	CHECK( n12.frac() == 2 );
	auto m12 = f1 * f2;
	CHECK( m12.digits() == 20 );
	CHECK( m12.digits() == m12.logical_digits() );
	CHECK( m12.frac() == 4 );

	auto p34 = f3 + f4;
	CHECK( p34.digits() == 32 );
	CHECK( p34.digits() == p34.logical_digits() );
	CHECK( p34.frac() == 10 );
	auto n34 = f3 - f4;
	CHECK( n34.digits() == 32 );
	CHECK( n34.digits() == n34.logical_digits() );
	CHECK( n34.frac() == 10 );
	auto m34 = f3 * f4;
	CHECK( m34.digits() == 64 );
	CHECK( m34.digits() == m34.logical_digits() );
	CHECK( m34.frac() == 20 );

	auto p24 = f2 + f4;
	CHECK( p24.digits() == 32 );
	CHECK( p24.digits() == p24.logical_digits() );
	CHECK( p24.frac() == 10 );
	auto n24 = f2 - f4;
	CHECK( n24.digits() == 32 );
	CHECK( n24.digits() == n24.logical_digits() );
	CHECK( n24.frac() == 10 );
	auto m24 = f2 * f4;
	CHECK( m24.digits() == 42 );
	CHECK( m24.digits() == m24.logical_digits() );
	CHECK( m24.frac() == 12 );

	WARTHOG_TEST_PDEF(5,48,16,0x655a'433e'0003'66ffLL);
	WARTHOG_TEST_PDEF(6,32,32,0x005a'5f33'1aef'f2eaLL);
	
	auto p56 = f5 + f6;
	CHECK( p56.digits() == 64 );
	CHECK( p56.logical_digits() == 64+16 );
	CHECK( p56.frac() == 32 );
	auto n56 = f5 - f6;
	CHECK( n56.digits() == 64 );
	CHECK( n56.logical_digits() == 64+16 );
	CHECK( n56.frac() == 32 );

#ifdef WARTHOG_INT128_ENABLED
	bool lt = f5 < f6;
	bool gt = f5 > f6;
	bool eq = f5 == f6;
	bool ne = f5 != f6;
	bool eq2 = f5 == f5;
	CHECK( lt == false );
	CHECK( gt == true );
	CHECK( eq == false );
	CHECK( ne == true );
	CHECK( eq2 == true );
#endif
}

}
