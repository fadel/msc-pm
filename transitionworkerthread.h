#ifndef REWINDWORKERTHREAD_H
#define REWINDWORKERTHREAD_H

#include "transitioncontrol.h"

#include <QObject>
#include <QThread>
#include <QEasingCurve>

class TransitionWorkerThread
    : public QThread
{
    Q_OBJECT
public:
    TransitionWorkerThread(TransitionControl *control);
    TransitionWorkerThread(TransitionControl *control, const QEasingCurve &easing);

    void setEasing(const QEasingCurve &easing);
    void run();

private:
    TransitionControl *m_control;
    QEasingCurve m_easing;
};

#endif // REWINDWORKERTHREAD_H
