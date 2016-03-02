#ifndef NUMERICRANGE_H
#define NUMERICRANGE_H

#include <iterator>
#include <stdexcept>

/*
 *  A (low memory usage) generator of ranges in steps of 1.
 */
template<typename T>
class NumericRange
{
public:
    class iterator
    {
        friend class NumericRange;
    public:
        typedef iterator self_type;
        typedef T value_type;
        typedef T& reference;
        typedef T* pointer;
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef int difference_type;

        iterator(const T &first): m_value(first) {}

        T operator *() const { return m_value; }
        const iterator &operator ++() {
            ++m_value;
            return *this;
        }
        iterator operator++(int) {
            iterator copy(*this);
            ++m_value;
            return copy;
        }
        const iterator &operator --() {
            --m_value;
            return *this;
        }
        iterator operator--(int) {
            iterator copy(*this);
            --m_value;
            return copy;
        }

        bool operator ==(const iterator &other) const {
            return m_value == other.m_value;
        }
        bool operator !=(const iterator &other) const {
            return m_value != other.m_value;
        }

    private:
        T m_value;
    };

    /*
     * The range [first, last).
     */
    NumericRange(const T &first, const T &last)
        : m_begin(first)
        , m_end(last)
    {
        if (first > last) {
            throw std::logic_error("first > last");
        }
    }

    typedef const iterator const_iterator;

    iterator begin() const { return m_begin; }
    iterator end() const   { return m_end; }

    const_iterator cbegin() const { return m_begin; }
    const_iterator cend() const   { return m_end; }

private:
    iterator m_begin, m_end;
};

#endif // NUMERICRANGE_H
