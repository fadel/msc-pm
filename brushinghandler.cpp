#include "brushinghandler.h"

BrushingHandler::BrushingHandler()
    : m_brushedItem(-1)
{
}

void BrushingHandler::clearBrush()
{
    brushItem(-1);
}

void BrushingHandler::brushItem(int item)
{
    m_brushedItem = item;
    emit itemBrushed(item);
}
