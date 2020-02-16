/* cdm_utils - v0.0 - c++ utility library - https://github.com/WubiCookie/cdm
   no warranty implied; use at your own risk

LICENSE

	   DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
				   Version 2, December 2004

Copyright (C) 2019 Charles Seizilles de Mazancourt <charles DOT de DOT mazancourt AT hotmail DOT fr>

Everyone is permitted to copy and distribute verbatim or modified
copies of this license document, and changing it is allowed as long
as the name is changed.

		   DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
  TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

 0. You just DO WHAT THE FUCK YOU WANT TO.

CREDITS

Written by Charles Seizilles de Mazancourt
*/

#ifndef CDM_UTILS_HPP
#define CDM_UTILS_HPP

#include <array>

namespace cdm
{
template<typename T>
struct colorLayoutRGBA
{
	T r, g, b, a;

	T getRed() const { return r; }
	T getGreen() const { return g; }
	T getBlue() const { return b; }
	T getAlpha() const { return a; }

	void setRed(T t) { r = t; }
	void setGreen(T t) { g = t; }
	void setBlue(T t) { b = t; }
	void setAlpha(T t) { a = t; }
};

template<typename T>
struct colorLayoutBGRA
{
	T b, g, r, a;

	T getRed() const { return r; }
	T getGreen() const { return g; }
	T getBlue() const { return b; }
	T getAlpha() const { return a; }

	void setRed(T t) { r = t; }
	void setGreen(T t) { g = t; }
	void setBlue(T t) { b = t; }
	void setAlpha(T t) { a = t; }
};

template<typename Layout>
struct color
{
	Layout layout;

	template<typename U>
	static color lerp(const color& begin, const color& end, U alpha);

	color operator+(const color& o) const;
	color operator-(const color& o) const;
	template<typename U>
	color operator*(U u) const;
};

using colorRGBA_uint8 = color<colorLayoutRGBA<uint8_t>>;
using colorBGRA_uint8 = color<colorLayoutBGRA<uint8_t>>;

template<typename T, size_t N>
struct uniform_gradient
{
	std::array<T, N> colors;

	template<typename U>
	T& operator[](U alpha)
	{
		size_t index = static_cast<size_t>(floor(alpha * (N-1)));

		if (index + 1 >= N)
			return colors[N - 1];

		T& c1 = colors[index];
		T& c2 = colors[index + 1];

		U tmp = alpha * (N - 1) - index;

		return T::lerp(c1, c2, tmp);
	}
};



template<typename T>
template<typename U>
static color<T> color<T>::lerp(const color<T>& begin, const color<T>& end, U alpha)
{
	U rb = begin.layout.getRed();
	U gb = begin.layout.getGreen();
	U bb = begin.layout.getBlue();
	U ab = begin.layout.getAlpha();

	U re = end.layout.getRed();
	U ge = end.layout.getGreen();
	U be = end.layout.getBlue();
	U ae = end.layout.getAlpha();

	re -= rb;
	ge -= gb;
	be -= bb;
	ae -= ab;

	re *= alpha;
	ge *= alpha;
	be *= alpha;
	ae *= alpha;

	re += rb;
	ge += gb;
	be += bb;
	ae += ab;

	color<T> res;
	res.layout.setRed(re);
	res.layout.setGreen(ge);
	res.layout.setBlue(be);
	res.layout.setAlpha(ae);

	return res;
}

template<typename T>
color<T> color<T>::operator+(const color<T>& o) const
{
	color<T> res = *this;
	res.layout.setRed(res.layout.getRed() - o.r);
	res.layout.setGreen(res.layout.getGreen() - o.g);
	res.layout.setBlue(res.layout.getBlue() - o.b);
	res.layout.setAlpha(res.layout.getAlpha() - o.a);

	return res;
}

template<typename T>
color<T> color<T>::operator-(const color<T>& o) const
{
	color<T> res = *this;
	res.layout.setRed(res.layout.getRed() + o.r);
	res.layout.setGreen(res.layout.getGreen() + o.g);
	res.layout.setBlue(res.layout.getBlue() + o.b);
	res.layout.setAlpha(res.layout.getAlpha() + o.a);

	return res;
}

template<typename T>
template<typename U>
color<T> color<T>::operator*(U u) const
{
	color<T> res = *this;
	res.layout.setRed(res.layout.getRed() * u);
	res.layout.setGreen(res.layout.getGreen() * u);
	res.layout.setBlue(res.layout.getBlue() * u);
	res.layout.setAlpha(res.layout.getAlpha() * u);

	return res;
}

/*template<typename T>
class span
{
	T* m_ptr = nullptr;
	size_t m_size = 0;

public:
	span() = default;
	span(T* ptr, size_t size) : m_ptr(ptr), m_size(size) {}
	span(std::vector<T>& v) : span(v.data(), v.size()) {}
	template<size_t N>
	span(std::array<T, N> a) : span(a.data(), a.size()) {}
	
	T* data() { return m_ptr; }
	const T* data() const { return m_ptr; }
	size_t size() const { return m_size; }

	bool empty() { return m_size == 0; }

	T* begin() { return m_ptr; }
	T* end() { return m_ptr + m_size; }
};//*/

template<typename Container>
auto find(Container& container, const typename Container::value_type& value)
{
	return std::find(container.begin(), container.end(), value);
}

template<typename Container, typename Predicate>
auto find_if(Container& container, Predicate predicate)
{
	return std::find_if(container.begin(), container.end(), predicate);
}

} // namespace cdm

#endif // CDM_UTILS_HPP