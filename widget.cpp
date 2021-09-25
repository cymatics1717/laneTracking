#include "widget.h"
#include <QPainter>
#include <QTimer>
#include <QDebug>
#include <QFileDialog>
widget::widget(QWidget *parent) : QWidget(parent),media(new mediaSource)
{
    connect(this,SIGNAL(seek(int)),media,SLOT(seek(int)));

    worker = new QThread;
    media->moveToThread(worker);
    connect(worker, SIGNAL(started()), media, SLOT(run()));
    connect(media,SIGNAL(incoming()),SLOT(incomingImage()));
    connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(worker, SIGNAL(finished()), media, SLOT(deleteLater()));

//    QTimer  *timer = new QTimer(this);
//    connect(timer,SIGNAL(timeout()),media,SLOT(run()));
    //    timer->start(30);

    setContextMenuPolicy(Qt::ActionsContextMenu);
    context_m = new QMenu(this);
    context_m->addAction("load Video sample",this,SLOT(loadSource()));


    media->setSource("");
    worker->start();

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
//    p.drawImage(0,media->currentImage().height(),media->BWImage());
    p.drawImage(media->currentImage().width(),0,media->BWImage());
}

void widget::mousePressEvent(QMouseEvent *event)
{
    qDebug()<<QString(100,'=') <<event->pos()<<","<<width();
//    emit seek(event->pos().x()*100/width());

//    context_m->exec(QCursor::pos());
}

void widget::incomingImage()
{
    update();
}

void widget::loadSource()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open File",
                                  "/home/wayne/Downloads", "Videos (*.avi *.mp4 *.mkv)");
    qDebug()<<fileName;
}
