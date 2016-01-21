#ifndef SELECTIONHANDLER_H
#define SELECTIONHANDLER_H

#include <set>
#include <vector>

#include <QObject>

class SelectionHandler
    : public QObject
{
    Q_OBJECT
public:
    SelectionHandler(int numItems);

signals:
    void selectionChanged(const std::vector<bool> &selection);

public slots:
    void setSelection(const std::vector<bool> &selection);
    void setSelected(int item, bool selected = true);
    void setSelected(const std::set<int> &items, bool selected = true);

private:
    std::vector<bool> m_selection;
};

#endif // SELECTIONHANDLER_H
