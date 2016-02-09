#include "transitioncontrol.h"

#include <QObject>
#include <QThread>

#include "rewindworkerthread.h"

// The mouse button used for interaction
static const Qt::MouseButton MOUSE_BUTTON = Qt::MiddleButton;

TransitionControl::TransitionControl()
    : m_t(1.0)
    , m_startPos(-1)
{
}

void TransitionControl::setT(double t)
{
    if (t < 0.0 || t > 1.0) {
        return;
    }

    m_t = t;
    emit tChanged(m_t);
}

void TransitionControl::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != MOUSE_BUTTON) {
        return;
    }

    m_t = 1.0;
    m_startPos = event->pos().x();
}

void TransitionControl::mouseMoveEvent(QMouseEvent *event)
{
    int x = event->pos().x();
    if (!(event->buttons() & MOUSE_BUTTON) || x > m_startPos || x < 0) {
        return;
    }

    m_t = double(x) / m_startPos;
    emit tChanged(m_t);
}

void TransitionControl::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != MOUSE_BUTTON) {
        return;
    }

    // Back to initial state
    m_startPos = -1;

    // We now have to smoothly go back to m_t == 1.0
    m_rewindThread = new RewindWorkerThread(this);
    connect(m_rewindThread, &QThread::finished, m_rewindThread, &QObject::deleteLater);
    m_rewindThread->start();
}
