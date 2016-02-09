#include "rewindworkerthread.h"

// The full duration (usecs) of the restoration animation
static const double DURATION = 250000;

// The time to wait (usecs) before the next animation tick
static const double TICK_TIME = DURATION / 60.0;

// The amount to increase 't' per time step
static const double TICK_SIZE = TICK_TIME / DURATION;

void RewindWorkerThread::run()
{
    double t = m_control->t();

    while (t + TICK_SIZE < 1.0) {
        t += TICK_SIZE;
        m_control->setT(t);
        QThread::usleep(TICK_TIME);
    }

    m_control->setT(1.0);
}
