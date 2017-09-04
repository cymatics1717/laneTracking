#ifndef MEDIASOURCE_H
#define MEDIASOURCE_H

#include <QDateTime>
#include <QImage>
#include <QObject>
#include "opencv2/opencv.hpp"

typedef bool (*vec4iComp) (const cv::Vec4i& lhs, const cv::Vec4i& rhs);

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
    const QImage& BWImage() const;
    void removeLines(const std::vector<cv::Vec4i>& lines, std::set<cv::Vec4i,vec4iComp> &out);
    void run();
    void stop();
    void seek(int pos);
private:
    cv::VideoCapture cap;
    QString source;
    QImage current;
    QImage bwImage;
    QDateTime epoch;
    int startframe;
};

#endif // MEDIASOURCE_H
