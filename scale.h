#ifndef SCALE_H
#define SCALE_H

#include <algorithm>

class Scale
{
public:
    Scale(float domainMin, float domainMax, float rangeMin, float rangeMax)
        : m_domainMin(domainMin)
        , m_domainMax(domainMax)
        , m_rangeMin(rangeMin)
        , m_rangeMax(rangeMax)
    {
        valuesUpdated();
    }

    virtual ~Scale() {}

    void setRangeMin(float rangeMin)   { m_rangeMin  = rangeMin; valuesUpdated(); }
    void setRangeMax(float rangeMax)   { m_rangeMax  = rangeMax; valuesUpdated(); }
    void setDomainMin(float domainMin) { m_domainMin = domainMin; valuesUpdated(); }
    void setDomainMax(float domainMax) { m_domainMax = domainMax; valuesUpdated(); }

    void setRange(float rangeMin, float rangeMax)
    {
        m_rangeMin = rangeMin;
        m_rangeMax = rangeMax;
        valuesUpdated();
    }

    void setDomain(float domainMin, float domainMax)
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

    virtual float operator()(float value) const = 0;

protected:
    // Called when internal values change
    virtual void valuesUpdated() {}

    float m_domainMin, m_domainMax;
    float m_rangeMin, m_rangeMax;
};

class LinearScale
    : public Scale
{
public:
    LinearScale(float domainMin, float domainMax, float rangeMin, float rangeMax)
        : Scale(domainMin, domainMax, rangeMin, rangeMax)
    {
        valuesUpdated();
    }

    virtual float operator()(float value) const
    {
        return (value - m_domainMin) * m_transformSlope + m_rangeMin;
    }

protected:
    virtual void valuesUpdated()
    {
        m_transformSlope = (m_rangeMax - m_rangeMin) / (m_domainMax - m_domainMin);
    }

private:
    float m_transformSlope;
};

#endif // SCALE_H
