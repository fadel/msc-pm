#ifndef SCALE_H
#define SCALE_H

#include <algorithm>

template<typename T>
class Scale
{
public:
    Scale(const T &domainMin, const T &domainMax, const T &rangeMin, const T &rangeMax)
        : m_domainMin(domainMin)
        , m_domainMax(domainMax)
        , m_rangeMin(rangeMin)
        , m_rangeMax(rangeMax)
    {
        valuesUpdated();
    }

    virtual ~Scale() {}

    void setRangeMin(T rangeMin)   { m_rangeMin  = rangeMin; valuesUpdated(); }
    void setRangeMax(T rangeMax)   { m_rangeMax  = rangeMax; valuesUpdated(); }
    void setDomainMin(T domainMin) { m_domainMin = domainMin; valuesUpdated(); }
    void setDomainMax(T domainMax) { m_domainMax = domainMax; valuesUpdated(); }

    void setRange(const T &rangeMin, const T &rangeMax)
    {
        m_rangeMin = rangeMin;
        m_rangeMax = rangeMax;
        valuesUpdated();
    }

    void setDomain(const T &domainMin, const T &domainMax)
    {
        m_domainMin = domainMin;
        m_domainMax = domainMax;
        valuesUpdated();
    }

    void inverse()
    {
        std::swap(m_rangeMin, m_domainMin);
        std::swap(m_rangeMax, m_domainMax);
        valuesUpdated();
    }

    virtual T operator()(const T &value) const = 0;

protected:
    // Called when internal values change
    virtual void valuesUpdated() {}

    T m_domainMin, m_domainMax;
    T m_rangeMin, m_rangeMax;
};

template<typename T>
class LinearScale
    : public Scale<T>
{
public:
    LinearScale(const T &domainMin, const T &domainMax, const T &rangeMin, const T &rangeMax)
        : Scale<T>(domainMin, domainMax, rangeMin, rangeMax)
    {
        valuesUpdated();
    }

    virtual T operator()(const T &value) const
    {
        return (value - Scale<T>::m_domainMin) * m_transformSlope + Scale<T>::m_rangeMin;
    }

protected:
    virtual void valuesUpdated()
    {
        m_transformSlope = (Scale<T>::m_rangeMax - Scale<T>::m_rangeMin)
                         / (Scale<T>::m_domainMax - Scale<T>::m_domainMin);
    }

private:
    T m_transformSlope;
};

#endif // SCALE_H
