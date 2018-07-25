/* Copyright (C) 2015-2016 INRA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef ORG_VLEPROJECT_EFYj_DYNARRAY_HPP
#define ORG_VLEPROJECT_EFYj_DYNARRAY_HPP

#include <EASTL/vector.h>

namespace efyj {

constexpr eastl::vector<int>::value_type size_max = { 512 };

class DynArrayView;

class DynArray
{
public:
    using iterator = eastl::vector<int>::iterator;
    using const_iterator = eastl::vector<int>::const_iterator;
    using value_type = eastl::vector<int>::value_type;
    using size_type = eastl::vector<int>::size_type;

private:
    eastl::vector<int> m_data;
    size_type m_line_size = { 0 };
    size_type m_capacity = { 0 };
    size_type m_size = { 0 };

    constexpr size_type compute_capacity(size_type rows)
    {
        return (1 + (rows / size_max)) * size_max;
    }

public:
    DynArray() = default;
    ~DynArray() = default;

    void init(size_type cols)
    {
        m_line_size = cols;
        m_capacity = size_max;
        m_size = 0;

        m_data.resize(m_capacity * m_line_size);
    }

    void init(size_type rows, size_type cols)
    {
        m_capacity = compute_capacity(rows);
        m_line_size = cols;
        m_size = rows;

        m_data.resize(m_capacity * m_line_size);
    }

    void push_line()
    {
        ++m_size;

        if (m_size > m_capacity) {
            m_capacity *= 3;
            m_capacity /= 2;
            m_data.resize(m_capacity * m_line_size);
        }
    }

    void pop_line()
    {
        if (m_size)
            --m_size;
    }

    template<typename Integer>
    int operator()(Integer row, Integer col) const
    {
        return m_data[row * m_line_size + col];
    }

    template<typename Integer>
    int& operator()(Integer row, Integer col)
    {
        return m_data[row * m_line_size + col];
    }

    void swap(DynArray& other)
    {
        using eastl::swap;

        swap(m_data, other.m_data);
        swap(m_line_size, other.m_line_size);
        swap(m_capacity, other.m_capacity);
        swap(m_size, other.m_size);
    }

    template<typename Integer>
    DynArrayView row(Integer row);

    template<typename Integer>
    DynArrayView row(Integer row) const;

    size_t rows() const
    {
        return m_size;
    }

    size_t cols() const
    {
        return m_line_size;
    }
};

class DynArrayView
{
public:
    using const_iterator = DynArray::const_iterator;
    using value_type = DynArray::value_type;

private:
    const_iterator m_first;
    const_iterator m_last;

public:
    DynArrayView(const_iterator first, const_iterator last)
      : m_first(first)
      , m_last(last)
    {}

    template<typename Integer>
    value_type operator[](Integer i) const
    {
        return *(m_first + i);
    }

    template<typename Integer>
    value_type operator[](Integer i)
    {
        return *(m_first + i);
    }
};

template<typename Integer>
inline DynArrayView
DynArray::row(Integer row)
{
    return DynArrayView(m_data.data() + (row * m_line_size),
                        m_data.data() + ((row + 1) * m_line_size));
}

template<typename Integer>
inline DynArrayView
DynArray::row(Integer row) const
{
    return DynArrayView(m_data.data() + (row * m_line_size),
                        m_data.data() + ((row + 1) * m_line_size));
}

} // namespace efyj

#endif
