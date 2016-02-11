#ifndef PROJECTIONHISTORY_H
#define PROJECTIONHISTORY_H

#include <QObject>

#include <armadillo>

class ProjectionHistory
    : public QObject
{
    Q_OBJECT
public:
    explicit ProjectionHistory(QObject *parent = 0);

    const arma::mat &Y() const     { return m_Y; }
    const arma::mat &first() const { return m_firstY; }
    const arma::mat &prev() const  { return m_prevY; }

    bool hasFirst() const { return m_hasFirst; }
    bool hasPrev() const  { return m_hasPrev; }

    void undo();
    void reset();

signals:
    void undoPerformed(const arma::mat &prevY) const;
    void resetPerformed(const arma::mat &firstY) const;
    void mapAdded(const arma::mat &newY) const;

public slots:
    void addMap(const arma::mat &Y);

private:
    arma::mat m_Y, m_firstY, m_prevY;
    bool m_hasFirst, m_hasPrev;
};

#endif // PROJECTIONHISTORY_H
