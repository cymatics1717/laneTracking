#ifndef WIDGET_H
#define WIDGET_H

#include <QMenu>
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
    void mouseReleaseEvent(QMouseEvent *event)  Q_DECL_OVERRIDE;
//    void resizeEvent(QResizeEvent *event)  Q_DECL_OVERRIDE;
public slots:
    void incomingImage();
    void loadSource();
signals:
    void seek(int);
private:
//    QSharedPointer<mediaSource> media;
    mediaSource *media;
    QThread *worker;

    QMenu *context_m;
};

#endif // WIDGET_H
