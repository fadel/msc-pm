#include "transitioncontrol.h"

#include <QObject>
#include <QThread>

#include "transitionworkerthread.h"

// The mouse button used for interaction
static const Qt::MouseButton MOUSE_BUTTON = Qt::RightButton;

TransitionControl::TransitionControl(QQuickItem *parent)
    : QQuickItem(parent)
    , m_t(1.0)
    , m_startPos(-1)
    , m_shouldRewind(false)
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
    if (event->button() != MOUSE_BUTTON
        || !(event->modifiers() & Qt::ControlModifier)) {
        event->ignore();
        return;
    }

    m_t = 1.0;
    m_startPos = event->pos().x();
}

void TransitionControl::mouseMoveEvent(QMouseEvent *event)
{
    int x = event->pos().x();
    if (!(event->buttons() & MOUSE_BUTTON) || x > m_startPos || x < 0) {
        event->ignore();
        return;
    }

    m_shouldRewind = true;
    m_t = double(x) / m_startPos;
    emit tChanged(m_t);
}

void TransitionControl::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != MOUSE_BUTTON) {
        event->ignore();
        return;
    }

    // Back to initial state
    m_startPos = -1;

    if (m_shouldRewind) {
        m_shouldRewind = false;

        // We now have to smoothly go back to m_t == 1.0
        m_rewindThread = new TransitionWorkerThread(this);
        connect(m_rewindThread, &QThread::finished, m_rewindThread, &QObject::deleteLater);
        m_rewindThread->start();
    }
}
