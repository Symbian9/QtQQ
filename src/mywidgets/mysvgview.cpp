#include "mysvgview.h"
#include <QPainter>
#include <QDebug>

MySvgView::MySvgView(QDeclarativeItem *parent) :
    QDeclarativeItem(parent)
{
    m_defaultSize = QSize(0,0);
    svg = new QSvgRenderer(this);
    connect (this, SIGNAL(widthChanged()), SLOT(onWidthChanged()));
    connect (this, SIGNAL(heightChanged()), SLOT(onHeightChanged()));
}

QUrl MySvgView::source() const
{
    return m_source;
}

QSize MySvgView::defaultSize() const
{
    return m_defaultSize;
}

void MySvgView::setDefaultSize(QSize arg)
{
    if(arg!=m_defaultSize){
        m_defaultSize = arg;
        emit defaultSizeChanged (arg);
    }
}

void MySvgView::onWidthChanged()
{
    setImplicitHeight (svg->defaultSize ().height ()*(width ()/svg->defaultSize ().width ()));
}

void MySvgView::onHeightChanged()
{
    setImplicitWidth (svg->defaultSize ().width ()*(height ()/svg->defaultSize ().height ()));
}

void MySvgView::paint(QPainter *painter)
{
    svg->render (painter);
}

void MySvgView::setSource(QUrl arg)
{
    if (m_source != arg) {
        m_source = arg;
        QString str;
        str = arg.toString ();

        if( str.mid (0, 3) == "qrc")
            str = str.mid (3, str.count ()-3);
        svg->load (str);
        setDefaultSize (svg->defaultSize ());
        int temp1 = svg->defaultSize ().width ();
        int temp2 = svg->defaultSize ().height ();
        if(width ()==0&&height ()==0){
            setImplicitWidth(temp1);//设置大小为默认大小
            setImplicitHeight(temp2);
        }else if(width()!=0)
            onWidthChanged();
        else if(height ()!=0)
            onHeightChanged ();
        update ();
        emit sourceChanged(arg);
    }
}
