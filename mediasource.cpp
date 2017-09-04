#include "mediasource.h"
#include <QDateTime>
#include <QDebug>
#include <QImage>
#include <QThread>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

static bool comp (const cv::Vec4i& lhs, const cv::Vec4i& rhs) {
    return lhs[1] + lhs[3]> rhs[1] + rhs[3];
}

mediaSource::mediaSource(QObject *parent) : mediaSource("",parent)
{
//
}

mediaSource::mediaSource(QString name,QObject *parent)
{
//    qDebug() << "source = "<<name << ;
    setSource(name);
}

int mediaSource::setSource(QString name)
{
    if(cap.isOpened()) cap.release();
    source = name;
    if(name.isEmpty()){
        cap.open(1);
    } else {
        cap.open(name.toStdString());
    }
    return cap.isOpened();
}

const QImage &mediaSource::currentImage() const
{
    return current;
}

const QImage &mediaSource::BWImage() const
{
    return bwImage;
}

void mediaSource::removeLines(const std::vector<cv::Vec4i>& lines, std::set<cv::Vec4i,vec4iComp> &out)
{
    std::set<cv::Vec4i,vec4iComp> left(comp),right(comp);
    int r_max_val = 80, r_min_val = 40;
    int l_max_val = 150, l_min_val = 100;

    double y_percent = 0.6;
    int max_x = current.width(), max_y = current.height();
    static double  last_left= 120;
    static double  last_right= 70;
    double left_avg_ang=0,right_avg_ang=0;
    for (size_t i = 0; i < lines.size(); i++)
    {
        cv::Vec4i l = lines[i];
        double theta_deg = atan2(l[2] - l[0], l[3] - l[1]) * 180 / CV_PI;
        if ((r_min_val < theta_deg && theta_deg < r_max_val) || (l_min_val < theta_deg && theta_deg < l_max_val))
        {
            if((l[3]>y_percent * max_y )&&(l[1])>y_percent * max_y){
//                out.insert(l);
                if(l[0]+l[2]<max_x){
                    left_avg_ang += theta_deg;
                    left.insert(l);
                } else {
                    right_avg_ang += theta_deg;
                    right.insert(l);
                }
            }
        }
    }
    left_avg_ang/=left.size();
    right_avg_ang/=right.size();


    double delta_percent = 0.001;
    {
        cv::Vec4i first = *left.begin();
        for(const cv::Vec4i& l :left){
            double theta_deg = atan2(l[0] - first[2], l[1] - first[3]) * 180 / CV_PI;
            if ((r_min_val < theta_deg && theta_deg < r_max_val) || (l_min_val < theta_deg && theta_deg < l_max_val)
                    || l[0]+l[2]-first[0]-first[2]<delta_percent*max_x)
            {
                out.insert(l);
            }
        }
    }
    {
        cv::Vec4i first = *right.begin();
        for(const cv::Vec4i& l :right){
            double theta_deg = atan2(l[0] - first[2], l[1] - first[3]) * 180 / CV_PI;
            if ((r_min_val < theta_deg && theta_deg < r_max_val) || (l_min_val < theta_deg && theta_deg < l_max_val)
                    || l[0]+l[2]-first[0]-first[2]<delta_percent*max_x)
            {
                out.insert(l);
            }
        }
    }

    std::cout<<left_avg_ang<<","<<right_avg_ang<<QString("\t[%1]/%2 ").arg(lines.size()).arg(out.size()).toStdString()<<std::endl;

}

void mediaSource::run()
{
    QDateTime last = QDateTime::currentDateTime();
    epoch = last;

    int total = cap.get(CV_CAP_PROP_FRAME_COUNT);

    startframe =0;
    // yellow lane
//    startframe =.45/18.33* total;
    // strips
//    startframe =3.3/18.33* total;
    // poor light
//    startframe =6.5/18.33* total;
    qDebug()<< total << cap.set(CV_CAP_PROP_POS_FRAMES, startframe);

    cv::Mat edges, buf, frame;
    cv::Size S = cv::Size(cap.get(CV_CAP_PROP_FRAME_WIDTH),cap.get(CV_CAP_PROP_FRAME_HEIGHT));
    cv::VideoWriter writer("/home/wayne/postProc.avi", CV_FOURCC('M', 'J', 'P', 'G'),cap.get(CV_CAP_PROP_FPS), S,true);

    while(cap.read(frame))
    {
        cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(35, 35));
        cv::morphologyEx(frame, buf, cv::MORPH_CLOSE, element);
//        cv::medianBlur(buf,buf,3);
//        cv::GaussianBlur(buf, buf, cv::Size(3,3), 0, 0);
        cv::bitwise_not(buf, edges);

        cv::cvtColor(edges, edges, CV_BGR2GRAY);
        cv::adaptiveThreshold(edges, edges,255,CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY,19,6);

//        element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
//        cv::morphologyEx(edges, edges, cv::MORPH_DILATE, element);
        cv::Canny(edges, edges, 0, 1, 3);

////////////////////////////////////////////////////////////////////////
        std::vector<cv::Vec4i> lines;
        std::set<cv::Vec4i,vec4iComp> out(comp);
        int min_len_val = 8;
        cv::HoughLinesP(edges, lines, 1, CV_PI / 180, 10, min_len_val, 0);
        removeLines(lines,out);
        for(const cv::Vec4i& l:out){
            cv::line(frame, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(255, 0, 255), 2, CV_AA);
        }
//        cv::line(frame, cv::Point(frame.cols/2, frame.rows), cv::Point(frame.cols/2, 0), cv::Scalar(255, 0, 255), 2, CV_AA);
//        cv::line(frame, cv::Point(0, frame.rows*.6), cv::Point(frame.cols, frame.rows*.6), cv::Scalar(255, 0, 255), 2, CV_AA);
//////////////////////////////////////////////////////////////////////////
        int framepos = cap.get(CV_CAP_PROP_POS_FRAMES);

        QDateTime now = QDateTime::currentDateTime();
        QString txt = QString("[%1/%2]:%3ms, avg fps = %4").arg(framepos,3).arg(total)
                  .arg(last.msecsTo(now),3).arg((framepos-startframe)*1000.0/epoch.msecsTo(now), 0, 'f', 2);
        last = now;

        cv::putText(frame,txt.toStdString(),cv::Point2d(0,30),CV_FONT_HERSHEY_COMPLEX, 1, cv::Scalar(255,0, 255));
        bwImage = QImage(edges.data, edges.cols, edges.rows, edges.step, QImage::Format_Grayscale8);
        cvtColor(frame, buf,CV_BGR2RGB);
        current = QImage(buf.data, buf.cols, buf.rows, buf.step, QImage::Format_RGB888);
        emit incoming();

//////////////////////////////////////////////////////////////////////////
        QThread::msleep(5);
        writer.write(frame);
    }
}

void mediaSource::stop()
{
    cap.release();
}

void mediaSource::seek(int pos)
{
//    if(pos<0||pos>100) return;
    int total = cap.get(CV_CAP_PROP_FRAME_COUNT);
//    double pos =3/18.33;
    qDebug()<<pos<< total << cap.set(CV_CAP_PROP_POS_FRAMES, pos*total/100);
}

