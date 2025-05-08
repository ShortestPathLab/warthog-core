#include <cmath>
#include <warthog/constants.h>
#include <warthog/geometry/geography.h>

#define PI_360 0.00872664625997
#define PI_180 0.017453292519943295

/* Writes result sine result sin(πa) to the location pointed to by sp
   Writes result cosine result cos(πa) to the location pointed to by cp

   In extensive testing, no errors > 0.97 ulp were found in either the sine
   or cosine results, suggesting the results returned are faithfully rounded.

   https://stackoverflow.com/a/42792940
*/

namespace warthog::geometry
{

namespace
{
void
sincospi(double a, double* sp, double* cp)
{
	double c, r, s, t, az;
	int64_t i;

	az = a * 0.0; // must be evaluated with IEEE-754 semantics
	/* for |a| >= 2**53, cospi(a) = 1.0, but cospi(Inf) = NaN */
	a = (fabs(a) < 9.0071992547409920e+15) ? a : az; // 0x1.0p53
	/* reduce argument to primary approximation interval (-0.25, 0.25) */
	r = nearbyint(a + a); // must use IEEE-754 "to nearest" rounding
	i = (int64_t)r;
	t = fma(-0.5, r, a);
	/* compute core approximations */
	s = t * t;
	/* Approximate cos(pi*x) for x in [-0.25,0.25] */
	r = -1.0369917389758117e-4;
	r = fma(r, s, 1.9294935641298806e-3);
	r = fma(r, s, -2.5806887942825395e-2);
	r = fma(r, s, 2.3533063028328211e-1);
	r = fma(r, s, -1.3352627688538006e+0);
	r = fma(r, s, 4.0587121264167623e+0);
	r = fma(r, s, -4.9348022005446790e+0);
	c = fma(r, s, 1.0000000000000000e+0);
	/* Approximate sin(pi*x) for x in [-0.25,0.25] */
	r = 4.6151442520157035e-4;
	r = fma(r, s, -7.3700183130883555e-3);
	r = fma(r, s, 8.2145868949323936e-2);
	r = fma(r, s, -5.9926452893214921e-1);
	r = fma(r, s, 2.5501640398732688e+0);
	r = fma(r, s, -5.1677127800499516e+0);
	s = s * t;
	r = r * s;
	s = fma(t, 3.1415926535897931e+0, r);
	/* map results according to quadrant */
	if(i & 2)
	{
		s = 0.0 - s; // must be evaluated with IEEE-754 semantics
		c = 0.0 - c; // must be evaluated with IEEE-754 semantics
	}
	if(i & 1)
	{
		t = 0.0 - s; // must be evaluated with IEEE-754 semantics
		s = c;
		c = t;
	}
	/* IEEE-754: sinPi(+n) is +0 and sinPi(-n) is -0 for positive integers n */
	if(a == floor(a)) s = az;
	*sp = s;
	*cp = c;
}

#ifdef NDEBUG
inline double
deg_to_rad(double deg)
{
	return deg * PI_180;
}
#else
double
deg_to_rad(double deg)
{
	return deg * M_PI / 180;
}
#endif

double
rad_to_deg(double rad)
{
	return fmod(rad * 180 / M_PI + 360, 360);
}

} // namespace

// Project the Earth onto a plane and into account the variation between
// meridians with latitude.
//
// https://en.wikipedia.org/wiki/Geographical_distance
double
spherical_distance(double lat_a, double lon_a, double lat_b, double lon_b)
{
	double p1 = deg_to_rad(lat_a);
	double l1 = deg_to_rad(lon_a);
	double p2 = deg_to_rad(lat_b);
	double l2 = deg_to_rad(lon_b);

	// (\Delta \phi) ^ 2
	double D_p   = p1 - p2;
	double D_p_2 = D_p * D_p;

	double cos_pm = cos((p1 + p2) / 2);
	double x      = cos_pm * (l1 - l2);

	return EARTH_RADIUS * sqrt(D_p_2 + x * x);
}

// Compute the distance between two points on a sphere by using the Haversine
// formula. The Great-Circle distance is only accurate up to 0.5% and is less
// so for short distances.
//
// https://en.wikipedia.org/wiki/Great-circle_distance
double
great_circle_distance(double lat_a, double lon_a, double lat_b, double lon_b)
{
	double p1 = deg_to_rad(lat_a);
	double l1 = deg_to_rad(lon_a);
	double p2 = deg_to_rad(lat_b);
	double l2 = deg_to_rad(lon_b);

	double D_l = fabs(l1 - l2);
	// double D_p = p1 - p2;
	double cos_p = cos(p1) * cos(p2);
	double sin_p = sin(p1) * sin(p2);

	double sigma = sin_p + cos_p * cos(D_l);

	return EARTH_RADIUS * acos(sigma);
}

// The Vincenty formula is an iterative procedure which can be accurate up to
// 0.5 mm. We use the special case where the ellipsoid is a sphere.
//
// https://en.wikipedia.org/wiki/Vincenty%27s_formulae
double
vincenty_distance(double lat_a, double lon_a, double lat_b, double lon_b)
{
	double p1 = deg_to_rad(lat_a);
	double l1 = deg_to_rad(lon_a);
	double p2 = deg_to_rad(lat_b);
	double l2 = deg_to_rad(lon_b);

	double D_l = fabs(l1 - l2);

	double cos_p1 = cos(p1);
	double cos_p2 = cos(p2);
	double sin_p1 = sin(p1);
	double sin_p2 = sin(p2);

	double x_a = cos_p2 * sin(D_l);
	double x_b = cos_p1 * sin_p2 - sin_p1 * cos_p2 * cos(D_l);
	double num = pow(x_a, 2) + pow(x_b, 2);

	double den = sin_p1 * sin_p2 + cos_p1 * cos_p2 * cos(D_l);

	return EARTH_RADIUS * atan(sqrt(num) / den);
}

// Exact (to a factor) Vincenty formula, accurate to .6 mm
//
// http://www.movable-type.co.uk/scripts/latlong-vincenty.html
double
exact_distance(double lat_a, double lon_a, double lat_b, double lon_b)
{
	double a = 6378.137;       // semi-major axis
	double b = 6356.752314245; // semi-minor axis
	double f = (a - b) / a;    // flattening
	// double inv_f = 298.257223563; // inverse flattening
	double p1 = deg_to_rad(lat_a);
	double l1 = deg_to_rad(lon_a);
	double p2 = deg_to_rad(lat_b);
	double l2 = deg_to_rad(lon_b);

	double D         = l2 - l1;
	double u1        = atan((1 - f) * tan(p1));
	double u2        = atan((1 - f) * tan(p2));
	double sin_u1    = sin(u1);
	double cos_u1    = cos(u1);
	double sin_u2    = sin(u2);
	double cos_u2    = cos(u2);
	double lambda    = D;
	double lambda_pi = 2 * M_PI;

	double cos2sigma_m  = 0;
	double cos_sq_alpha = 0;
	double sigma        = 0;
	double sin_sigma    = 0;
	double cos_sigma    = 0;

	while(fabs(lambda - lambda_pi) > 1e-12)
	{
		double sin_lambda = sin(lambda);
		double cos_lambda = cos(lambda);
		sin_sigma         = sqrt(
            (cos_u2 * sin_lambda) * (cos_u2 * sin_lambda)
            + (cos_u1 * sin_u2 - sin_u1 * cos_u2 * cos_lambda)
                * (cos_u1 * sin_u2 - sin_u1 * cos_u2 * cos_lambda));
		cos_sigma = sin_u1 * sin_u2 + cos_u1 * cos_u2 * cos_lambda;
		sigma     = atan2(sin_sigma, cos_sigma);

		double alpha = asin(cos_u1 * cos_u2 * sin_lambda / sin_sigma);

		cos_sq_alpha = cos(alpha) * cos(alpha);
		cos2sigma_m  = cos_sigma - 2 * sin_u1 * sin_u2 / cos_sq_alpha;

		double cc = f / 16 * cos_sq_alpha * (4 + f * (4 - 3 * cos_sq_alpha));
		lambda_pi = lambda;
		lambda    = D
		    + (1 - cc) * f * sin(alpha)
		        * (sigma
		           + cc * sin_sigma
		               * (cos2sigma_m
		                  + cc * cos_sigma
		                      * (-1 + 2 * cos2sigma_m * cos2sigma_m)));
	}

	double usq = cos_sq_alpha * (a * a - b * b) / (b * b);
	double aa
	    = 1 + usq / 16384 * (4096 + usq * (-768 + usq * (320 - 175 * usq)));
	double bb = usq / 1024 * (256 + usq * (-128 + usq * (74 - 47 * usq)));
	double delta_sigma = bb * sin_sigma
	    * (cos2sigma_m
	       + bb / 4
	           * (cos_sigma * (-1 + 2 * cos2sigma_m * cos2sigma_m)
	              - bb / 6 * cos2sigma_m * (-3 + 4 * sin_sigma * sin_sigma)
	                  * (-3 + 4 * cos2sigma_m * cos2sigma_m)));

	// length of the geodesic
	return b * aa * (sigma - delta_sigma);
}

// fast haversine from
// https://developer.nvidia.com/blog/fast-great-circle-distance-calculation-cuda-c/
double
fast_haversine(double lat1, double lon1, double lat2, double lon2)
{
	double c1, c2, dlat, dlon, d1, d2, dunce;

	sincospi(lat1 / 180, &dunce, &c1);
	sincospi(lat2 / 180, &dunce, &c2);
	dlat = lat2 - lat1;
	dlon = lon2 - lon1;
	sincospi(dlat / 360, &d1, &dunce);
	sincospi(dlon / 360, &d2, &dunce);
	double t = d2 * d2 * c1 * c2;
	double a = d1 * d1 + t;

	// 2*R*asin...
	return TWO_EARTH_RADII * asin(sqrt(a));
}

// Approximate haversine function, meant for speed (again)
// https://blog.utoctadel.com.ar/2016/05/20/fast-haversine.html
double
haversine_approx(double lat1, double lon1, double lat2, double lon2)
{
	double l1 = cos((lat1 + lat2) * PI_360);
	double Dl = fabs(lat1 - lat2) * PI_360;
	double Dp = fabs(lon1 - lon2) * PI_360;
	double f  = Dl * Dl + l1 * l1 * Dp * Dp;
	double c  = atan2(sqrt(f), sqrt(1 - f));

	return TWO_EARTH_RADII * c;
}

double
haversine(double lat1, double lon1, double lat2, double lon2)
{
	double p_1   = deg_to_rad(lat1);
	double p_2   = deg_to_rad(lat2);
	double D_p   = p_1 - p_2;
	double D_l   = deg_to_rad(lon2 - lon1);
	double hav_l = sin(D_l / 2);
	hav_l       *= hav_l;
	double hav_p = sin(D_p / 2);
	hav_p       *= hav_p;
	double a     = cos(p_1) * cos(p_2) * hav_l;

	// 2*R*asin...
	return TWO_EARTH_RADII * asin(sqrt(hav_p + a));
}

// We calculate bearing with the following formula:
// θ = atan2( sin Δλ ⋅ cos φ2 , cos φ1 ⋅ sin φ2 − sin φ1 ⋅ cos φ2 ⋅ cos Δλ )
// where:
//   φ1,λ1 is the start point,
//   φ2,λ2 the end point
//   Δλ is the difference in longitude
double
get_bearing(double lat_a, double lng_a, double lat_b, double lng_b)
{
	double l1 = deg_to_rad(lng_a);
	double l2 = deg_to_rad(lng_b);
	double p1 = deg_to_rad(lat_a);
	double p2 = deg_to_rad(lat_b);
	double y  = sin(l2 - l1) * cos(p2);
	double x  = cos(p1) * sin(p2) - sin(p1) * cos(p2) * cos(l2 - l1);

	return rad_to_deg(std::atan2(y, x));
}

// We use the magnetic North (86°26′52.8″N 175°20′45.06″E) as to avoid angles
// being 180° for all points.
double
true_bearing(double lat, double lng)
{
	return get_bearing(
	    deg_to_rad(86.448), deg_to_rad(175.5968), deg_to_rad(lng),
	    deg_to_rad(lat));
}

// The angle (ABC) is defined as the bearing from C to A when B is the origin.
double
get_angle(
    double lat_a, double lng_a, double lat_b, double lng_b, double lat_c,
    double lng_c)
{
	return get_bearing(
	    lat_a - lat_b, lng_a - lng_b, lat_c - lat_b, lng_c - lng_b);
}

// This works by checking that the longitude of A and C differ in sign wrt B.
bool
between(
    double lat_a, double lng_a, double lat_b, double lng_b, double lat_c,
    double lng_c)
{
	return (lng_a - lng_b) * (lng_c - lng_b) < 0;
}

// Variant of 'between' where we pass the origin.
bool
between(
    double lat_o, double lng_o, double lat_a, double lng_a, double lat_b,
    double lng_b, double lat_c, double lng_c)
{
	double lng_sa = lng_o - lng_a;
	double lng_sb = lng_o - lng_b;
	double lng_sc = lng_o - lng_c;
	// TODO Offset latitudes too?
	return between(lat_a, lng_sa, lat_b, lng_sb, lat_c, lng_sc);
}

double
get_bearing_xy(uint32_t lat1, uint32_t lng1, uint32_t lat2, uint32_t lng2)
{
	return get_bearing(
	    lat1 / warthog::DIMACS_RATIO, lng1 / warthog::DIMACS_RATIO,
	    lat2 / warthog::DIMACS_RATIO, lng2 / warthog::DIMACS_RATIO);
}

double
true_bearing_xy(uint32_t lng, uint32_t lat)
{
	return true_bearing(
	    lat / warthog::DIMACS_RATIO, lng / warthog::DIMACS_RATIO);
}

double
get_angle_xy(
    uint32_t lat_a, uint32_t lng_a, uint32_t lat_b, uint32_t lng_b,
    uint32_t lat_c, uint32_t lng_c)
{
	return get_angle(
	    lat_a / warthog::DIMACS_RATIO, lng_a / warthog::DIMACS_RATIO,
	    lat_b / warthog::DIMACS_RATIO, lng_b / warthog::DIMACS_RATIO,
	    lat_c / warthog::DIMACS_RATIO, lng_c / warthog::DIMACS_RATIO);
}

bool
between_xy(
    uint32_t lat_a, uint32_t lng_a, uint32_t lat_b, uint32_t lng_b,
    uint32_t lat_c, uint32_t lng_c)
{
	return between(
	    lat_a / warthog::DIMACS_RATIO, lng_a / warthog::DIMACS_RATIO,
	    lat_b / warthog::DIMACS_RATIO, lng_b / warthog::DIMACS_RATIO,
	    lat_c / warthog::DIMACS_RATIO, lng_c / warthog::DIMACS_RATIO);
}

bool
between_xy(
    uint32_t lat_s, uint32_t lng_s, uint32_t lat_a, uint32_t lng_a,
    uint32_t lat_b, uint32_t lng_b, uint32_t lat_c, uint32_t lng_c)
{
	return between(
	    lat_s / warthog::DIMACS_RATIO, lng_s / warthog::DIMACS_RATIO,
	    lat_a / warthog::DIMACS_RATIO, lng_a / warthog::DIMACS_RATIO,
	    lat_b / warthog::DIMACS_RATIO, lng_b / warthog::DIMACS_RATIO,
	    lat_c / warthog::DIMACS_RATIO, lng_c / warthog::DIMACS_RATIO);
}

} // namespace warthog::geometry
