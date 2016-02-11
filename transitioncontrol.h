#ifndef TRANSITIONCONTROL_H
#define TRANSITIONCONTROL_H

#include <QQuickItem>

/*
 * This component emits signals indicating how far from its left edge is the
 * mouse since the mouse button was pressed (starting from t == 1.0 with 0.0
 * being exactly at the left edge). As the mouse is released, it emits periodic
 * signals incrementing the value until it is restored to the default.
 */
class TransitionControl :
    public QQuickItem
{
    Q_OBJECT
public:
    TransitionControl(QQuickItem *parent = 0);
    double t() const { return m_t; }

signals:
    void tChanged(double t) const;

public slots:
    void setT(double t);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private:
    double m_t;

    // The x pos where interaction started
    int m_startPos;

    bool m_shouldRewind;

    // Controls the smooth rewind transition
    QThread *m_rewindThread;
};

#endif // TRANSITIONCONTROL_H
