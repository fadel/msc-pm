#ifndef BRUSHINGHANDLER_H
#define BRUSHINGHANDLER_H

#include <QObject>

class BrushingHandler
    : public QObject
{
    Q_OBJECT
public:
    BrushingHandler();

    void clearBrush();

signals:
    void itemBrushed(int item) const;

public slots:
    void brushItem(int item);

private:
    int m_brushedItem;
};

#endif // BRUSHINGHANDLER_H
