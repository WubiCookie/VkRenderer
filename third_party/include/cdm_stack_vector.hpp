/* cdm_stack_vector - v0.0 - c++ container - https://github.com/WubiCookie/cdm
   no warranty implied; use at your own risk

LICENSE

	   DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
				   Version 2, December 2004

Copyright (C) 2020 Charles Seizilles de Mazancourt <charles DOT de DOT mazancourt AT hotmail DOT fr>

Everyone is permitted to copy and distribute verbatim or modified
copies of this license document, and changing it is allowed as long
as the name is changed.

		   DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
  TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

 0. You just DO WHAT THE FUCK YOU WANT TO.

CREDITS

Written by Charles Seizilles de Mazancourt
*/

#ifndef CDM_STACKVECTOR_HPP
#define CDM_STACKVECTOR_HPP

#include <array>
#include <stdexcept>

namespace cdm
{
template<typename T, size_t N>
class stack_vector
{
	std::array<T, N> m_array{};
	size_t m_size = 0;

public:
	size_t size() const noexcept { return m_size; }
	T* data() noexcept { return m_array.data(); }
	const T* data() const noexcept { return m_array.data(); }

	auto begin() noexcept { return m_array.begin(); }
	auto end() noexcept { return m_array.end(); }

	void push_back(const T& t)
	{
		if (m_size >= N)
			throw std::runtime_error("could not push back, container is full (" + std::to_string(N) + ")");

		m_array[m_size] = t;
		m_size++;
	}

};

} // namespace cdm

#endif // CDM_STACKVECTOR_HPP