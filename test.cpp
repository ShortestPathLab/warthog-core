#include <iostream>

enum class X {
	a,
	b,
	c
};

template <X a>
concept is_c = a == X::c;

int main()
{
	int a;
	std::cin >> a;
	if constexpr (is_c<static_cast<X>(a)>) {
		std::cout << "done\n";
	} else {
		std::cout << "bad\n";
	}
}
