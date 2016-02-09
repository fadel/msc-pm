#ifndef REWINDWORKERTHREAD_H
#define REWINDWORKERTHREAD_H

#include "transitioncontrol.h"

#include <QObject>
#include <QThread>

class RewindWorkerThread
    : public QThread
{
    Q_OBJECT
public:
    RewindWorkerThread(TransitionControl *control) { m_control = control; }
    void run();

private:
    TransitionControl *m_control;
};

#endif // REWINDWORKERTHREAD_H
