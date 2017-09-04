#include "widget.h"
#include <QPainter>
#include <QTimer>
#include <QDebug>
widget::widget(QWidget *parent) : QWidget(parent),media(new mediaSource)
{
    connect(this,SIGNAL(seek(int)),media,SLOT(seek(int)));
    media->setSource("/home/wayne/Downloads/highwayKR.avi");

    worker = new QThread;
    media->moveToThread(worker);
    connect(worker, SIGNAL(started()), media, SLOT(run()));
    connect(media,SIGNAL(incoming()),SLOT(incomingImage()));
    connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(worker, SIGNAL(finished()), media, SLOT(deleteLater()));
    worker->start();
//    QTimer  *timer = new QTimer(this);
//    connect(timer,SIGNAL(timeout()),media,SLOT(run()));
    //    timer->start(30);
}

widget::~widget()
{
    media->stop();
//    qDebug()<< media;
//    delete media;
}

void widget::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    p.drawImage(0,0,media->currentImage());
    p.drawImage(media->currentImage().width(),0,media->BWImage());
}

void widget::mousePressEvent(QMouseEvent *event)
{
    qDebug()<<QString(100,'=') <<event->pos()<<","<<width();
    emit seek(event->pos().x()*100/width());
}

void widget::incomingImage()
{
    update();
}
