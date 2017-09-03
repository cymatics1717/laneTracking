#ifndef WIDGET_H
#define WIDGET_H

#include <QPaintEvent>
#include <QThread>
#include <QWidget>
#include <memory>
#include "mediasource.h"
class widget : public QWidget
{
    Q_OBJECT
public:
    explicit widget(QWidget *parent = nullptr);
    ~widget();
protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event)  Q_DECL_OVERRIDE;
public slots:
    void incomingImage();
private:
//    QSharedPointer<mediaSource> media;
    mediaSource *media;
    QThread *worker;
};

#endif // WIDGET_H
