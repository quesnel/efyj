/* Copyright (C) 2016-2017 INRA
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

#ifndef ORG_VLEPROJECT_EFYJ_MATRIX_HPP
#define ORG_VLEPROJECT_EFYJ_MATRIX_HPP

#include <stdexcept> // std::out_of_range exception
#include <vector>    // default container type.

namespace efyj {

/**
 * An \e matrix defined a two-dimensional template array. Informations are
 * stored into a \e std::vector<T> by default.
 *
 * \tparam T Type of element
 * \tparam Containre Type of container to store two-dimensional array.
 */
template<typename T, class Container = std::vector<T>>
class matrix
{
public:
    using container_type = Container;
    using value_type = T;
    using reference = typename container_type::reference;
    using const_reference = typename container_type::const_reference;
    using iterator = typename container_type::iterator;
    using const_iterator = typename container_type::const_iterator;
    using reverse_iterator = typename container_type::reverse_iterator;
    using const_reverse_iterator =
      typename container_type::const_reverse_iterator;
    using size_type = typename container_type::size_type;

protected:
    container_type m_c;
    size_type m_rows, m_columns;

public:
    matrix();

    explicit matrix(size_type cols, size_type rows);
    explicit matrix(size_type cols, size_type rows, const value_type& value);

    ~matrix() = default;

    matrix(const matrix& q) = default;
    matrix(matrix&& q) = default;

    matrix& operator=(const matrix& q) = default;
    matrix& operator=(matrix&& q) = default;

    void resize(size_type cols, size_type rows);
    void resize(size_type cols, size_type rows, const value_type& value);

    iterator begin() noexcept;
    const_iterator begin() const noexcept;
    iterator end() noexcept;
    const_iterator end() const noexcept;

    reverse_iterator rbegin() noexcept;
    const_reverse_iterator rbegin() const noexcept;
    reverse_iterator rend() noexcept;
    const_reverse_iterator rend() const noexcept;

    const_iterator cbegin() const noexcept;
    const_iterator cend() const noexcept;
    const_reverse_iterator crbegin() const noexcept;
    const_reverse_iterator crend() const noexcept;

    bool empty() const noexcept;
    size_type size() const noexcept;

    size_type rows() const noexcept;
    size_type columns() const noexcept;

    void set(size_type col, size_type row, const value_type& x);
    void set(size_type col, size_type row, value_type&& x);

    template<class... Args>
    void emplace(size_type col, size_type row, Args&&... args);

    const_reference operator()(size_type col, size_type row) const;
    reference operator()(size_type col, size_type row);

    void swap(matrix& c) noexcept(noexcept(m_c.swap(c.m_c)));

private:
    void m_check_index(size_type col, size_type) const;
};

template<typename T, class Container>
matrix<T, Container>::matrix()
  : m_c()
  , m_rows(0)
  , m_columns(0)
{}

template<typename T, class Container>
matrix<T, Container>::matrix(size_type columns, size_type rows)
  : m_c(rows * columns)
  , m_rows(rows)
  , m_columns(columns)
{}

template<typename T, class Container>
matrix<T, Container>::matrix(size_type columns,
                             size_type rows,
                             const value_type& value)
  : m_c(rows * columns, value)
  , m_rows(rows)
  , m_columns(columns)
{}

template<typename T, class Container>
void
matrix<T, Container>::resize(size_type cols, size_type rows)
{
    container_type new_c(rows * cols);

    size_type rmin = std::min(rows, m_rows);
    size_type cmin = std::min(cols, m_columns);

    for (size_type r = 0; r != rmin; ++r)
        for (size_type c = 0; c != cmin; ++c)
            new_c[r * cols + c] = m_c[r * m_columns + c];

    m_columns = cols;
    m_rows = rows;
    std::swap(new_c, m_c);
}

template<typename T, class Container>
void
matrix<T, Container>::resize(size_type cols,
                             size_type rows,
                             const value_type& value)
{
    m_c.resize(rows * cols);
    m_rows = rows;
    m_columns = cols;

    std::fill(std::begin(m_c), std::end(m_c), value);
}

template<typename T, class Container>
typename matrix<T, Container>::iterator
matrix<T, Container>::begin() noexcept
{
    return m_c.begin();
}

template<typename T, class Container>
typename matrix<T, Container>::const_iterator
matrix<T, Container>::begin() const noexcept
{
    return m_c.begin();
}

template<typename T, class Container>
typename matrix<T, Container>::iterator
matrix<T, Container>::end() noexcept
{
    return m_c.end();
}

template<typename T, class Container>
typename matrix<T, Container>::const_iterator
matrix<T, Container>::end() const noexcept
{
    return m_c.end();
}

template<typename T, class Container>
typename matrix<T, Container>::reverse_iterator
matrix<T, Container>::rbegin() noexcept
{
    return m_c.rbegin();
}

template<typename T, class Container>
typename matrix<T, Container>::const_reverse_iterator
matrix<T, Container>::rbegin() const noexcept
{
    return m_c.rbegin();
}

template<typename T, class Container>
typename matrix<T, Container>::reverse_iterator
matrix<T, Container>::rend() noexcept
{
    return m_c.rend();
}

template<typename T, class Container>
typename matrix<T, Container>::const_reverse_iterator
matrix<T, Container>::rend() const noexcept
{
    return m_c.rend();
}

template<typename T, class Container>
typename matrix<T, Container>::const_iterator
matrix<T, Container>::cbegin() const noexcept
{
    return m_c.cbegin();
}

template<typename T, class Container>
typename matrix<T, Container>::const_iterator
matrix<T, Container>::cend() const noexcept
{
    return m_c.cend();
}

template<typename T, class Container>
typename matrix<T, Container>::const_reverse_iterator
matrix<T, Container>::crbegin() const noexcept
{
    return m_c.crbegin();
}

template<typename T, class Container>
typename matrix<T, Container>::const_reverse_iterator
matrix<T, Container>::crend() const noexcept
{
    return m_c.crend();
}

template<typename T, class Container>
bool
matrix<T, Container>::empty() const noexcept
{
    return m_c.empty();
}

template<typename T, class Container>
typename matrix<T, Container>::size_type
matrix<T, Container>::size() const noexcept
{
    return m_c.size();
}

template<typename T, class Container>
typename matrix<T, Container>::size_type
matrix<T, Container>::rows() const noexcept
{
    return m_rows;
}

template<typename T, class Container>
typename matrix<T, Container>::size_type
matrix<T, Container>::columns() const noexcept
{
    return m_columns;
}

template<typename T, class Container>
void
matrix<T, Container>::set(size_type column, size_type row, const value_type& x)
{
    m_check_index(column, row);
    m_c[row * m_columns + column] = x;
}

template<typename T, class Container>
void
matrix<T, Container>::set(size_type column, size_type row, value_type&& x)
{
    m_check_index(column, row);
    m_c.emplace(std::begin(m_c) + (row * m_columns + column), std::move(x));
}

template<typename T, class Container>
template<class... Args>
void
matrix<T, Container>::emplace(size_type column, size_type row, Args&&... args)
{
    m_check_index(column, row);
    m_c.emplace(std::begin(m_c) + (row * m_columns + column),
                std::forward<Args>(args)...);
}

template<typename T, class Container>
typename matrix<T, Container>::const_reference
matrix<T, Container>::operator()(size_type column, size_type row) const
{
    m_check_index(column, row);
    return m_c[row * m_columns + column];
}

template<typename T, class Container>
typename matrix<T, Container>::reference
matrix<T, Container>::operator()(size_type column, size_type row)
{
    m_check_index(column, row);
    return m_c[row * m_columns + column];
}

template<typename T, class Container>
void
matrix<T, Container>::swap(matrix& c) noexcept(noexcept(m_c.swap(c.m_c)))
{
    std::swap(m_c, c.m_c);
    std::swap(m_columns, c.m_columns);
    std::swap(m_rows, c.m_rows);
}

template<typename T, class Container>
void
matrix<T, Container>::m_check_index(size_type column, size_type row) const
{
    if (column >= m_columns or row >= m_rows)
#ifdef __func__
        throw std::out_of_range(__func__);
#else
        throw std::out_of_range("matrx::m_check_index");
#endif
}
}

#endif
