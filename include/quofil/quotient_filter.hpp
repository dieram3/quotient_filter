// The MIT License (MIT)
//
// Copyright (c) <June 2015> <Diego Ramirez and Marcello Tavano>
// any issue can mail to <(diego.ramirezd and marcello.tavanolanas)@mail.udp.cl>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
/// \file
/// \brief Contains the quotient_filter template class.

#ifndef QUOFIL_QUOTIENT_FILTER_HPP
#define QUOFIL_QUOTIENT_FILTER_HPP

#include <functional> // for std::hash, std::equal_to
#include <memory>     // for std::unique_ptr

namespace quofil {

using uchar = unsigned char;

namespace detail {

class quotient_filter_table {
public:
  quotient_filter_table(std::size_t q, std::size_t r);
  void insert(std::size_t hash_value);
  bool contains(std::size_t hash_value) const;
  void erase(std::size_t hash_value);

private:
  std::size_t q;          // Number of bits that holds the quotient part.
  std::size_t r;          // Number of bits that holds the remainder part.
  std::size_t table_size; // Number of allocated bytes.
  std::unique_ptr<uchar[]> table; // The hash table
};

} // end namespace detail

template <class Key, class Hash = std::hash<Key>,
          class Allocator = std::allocator<Key>>
class quotient_filter {
public:
  void insert(const Key &elem);
  void erase(const Key &elem);
  bool contains(const Key &elem) const;
  double error_probability() const;
};

} // end namespace quofil

#endif // Header guard
