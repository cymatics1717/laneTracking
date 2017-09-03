#ifndef MEDIASOURCE_H
#define MEDIASOURCE_H

#include <QDateTime>
#include <QImage>
#include <QObject>
#include "opencv2/opencv.hpp"
#include "LaneDetection.h"

class mediaSource : public QObject
{
    Q_OBJECT
public:
    explicit mediaSource(QObject *parent = nullptr);
    mediaSource(QString name,QObject *parent = nullptr);
signals:
    void incoming();
public slots:
    int setSource(QString source);
    const QImage& currentImage() const;
    void run();
    void stop();
private:
    cv::VideoCapture cap;
    QString source;
    QImage current;
    QDateTime epoch;
    LaneDetection *ld;
};

#endif // MEDIASOURCE_H
