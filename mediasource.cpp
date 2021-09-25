#include "mediasource.h"
#include <QDateTime>
#include <QDebug>
#include <QImage>
#include <QThread>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/videoio.hpp"

static cv::Point2f median(const cv::Vec4i& line){
    return cv::Point2f((line[0]+line[2])/2,(line[1]+line[3])/2);
}
static double euclidDist(const cv::Vec4i& line)
{
    return static_cast<double>((line[0] - line[2]) * (line[0] - line[2]) + (line[1] - line[3]) * (line[1] - line[3]));
}
static double angle(const cv::Vec4i& line)
{
    return atan2(line[2] - line[0], line[3] - line[1]) * 180 / CV_PI;
}

static bool comp (const cv::Vec4i& lhs, const cv::Vec4i& rhs) {
    return euclidDist(lhs)> euclidDist(rhs);
}

static bool compAngle (const cv::Vec4i& lhs, const cv::Vec4i& rhs) {
    return angle(lhs)> angle(rhs);
}

mediaSource::mediaSource(QObject *parent) : mediaSource("",parent)
{
//
}

mediaSource::mediaSource(QString name,QObject *parent)
{
//    qDebug() << "source = "<<name << ;
//    setSource(name);
}

int mediaSource::setSource(QString name)
{
    if(cap.isOpened()) cap.release();
    source = name;
    if(name.isEmpty()){
        cap.open(0);
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

void mediaSource::splitLines(const std::vector<cv::Vec4i> &lines, dumpbin &dump,curves &out, double min_door, double max_door)
{
    int r_max_val = 80, r_min_val = 40;
    int l_max_val = 150, l_min_val = 100;

    double y_percent = 0.6;
    int max_x = current.width(), max_y = current.height();
    for (size_t i = 0; i < lines.size(); i++)
    {
        cv::Vec4i l = lines[i];
        double theta_deg = angle(l);
        if ((r_min_val < theta_deg && theta_deg < r_max_val) || (l_min_val < theta_deg && theta_deg < l_max_val))
        {
            if((l[3]>y_percent * max_y )&&(l[1])>y_percent * max_y){
                dump.emplace(l,theta_deg);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<cv::Point2f>> groups;
    double pivot = std::numeric_limits<double>::max();
    std::vector<cv::Point2f> tmp;
    for(auto x:dump){
        if(pivot-x.second>=max_door){
            if(tmp.size()>0){
                groups.push_back(tmp);
                cv::Vec4f line;
                cv::fitLine(tmp,line, cv::DIST_L2,0,0.01,0.01);
                out.push_back(line);
            }
            tmp.clear();
//            tmp.push_back(median(x.first));
            tmp.push_back(cv::Point2f(x.first[0],x.first[1]));
            tmp.push_back(cv::Point2f(x.first[2],x.first[3]));
        } else if(pivot-x.second<min_door){
//            tmp.push_back(median(x.first));
            tmp.push_back(cv::Point2f(x.first[0],x.first[1]));
            tmp.push_back(cv::Point2f(x.first[2],x.first[3]));
        }
        pivot = x.second;
    }
    std::cout<<groups.size()<<std::endl;
    ////////////////////////////////////////////////////////////////////////

}

static void drawLine(cv::Mat& image, double theta, double rho, cv::Scalar color,double cap)
{
    if (theta < CV_PI/4. || theta > 3.*CV_PI/4.)// ~vertical line
    {
       cv::Point pt1(rho/cos(theta), 0);
       cv::Point pt2((rho - cap * sin(theta))/cos(theta), cap);
       cv::line( image, pt1, pt2, color, 6);
    }
    else
    {
       cv::Point pt1(0, rho/sin(theta));
       cv::Point pt2(image.cols, (rho - image.cols * cos(theta))/sin(theta));
       cv::line(image, pt1, pt2, cv::Scalar(255,0,0), 6);
    }
}

void mediaSource::run()
{
    QDateTime last = QDateTime::currentDateTime();
    epoch = last;

    int total = cap.get(cv::CAP_PROP_FRAME_COUNT);

    startframe =0;
    // yellow lane
//    startframe =.45/18.33* total;
    // strips
//    startframe =3.3/18.33* total;
    // poor light
//    startframe =6.5/18.33* total;
    qDebug()<< total << cap.set(cv::CAP_PROP_POS_FRAMES, startframe);

    cv::Mat edges, buf, frame;
    cv::Size S = cv::Size(cap.get(cv::CAP_PROP_FRAME_WIDTH),cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    cv::VideoWriter writer;
    writer.open("/home/wayne/postProc.avi", writer.fourcc('M', 'J', 'P', 'G'),cap.get(cv::CAP_PROP_FPS), S,true);

    while(cap.read(frame))
    {
        cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(35, 35));
        cv::morphologyEx(frame, buf, cv::MORPH_CLOSE, element);
//        cv::medianBlur(buf,buf,3);
//        cv::GaussianBlur(buf, buf, cv::Size(3,3), 0, 0);
        cv::bitwise_not(buf, edges);

        cv::cvtColor(edges, edges, cv::COLOR_BGR2GRAY);
        cv::adaptiveThreshold(edges, edges,255,cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY,19,6);

//        element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
//        cv::morphologyEx(edges, edges, cv::MORPH_DILATE, element);
        cv::Canny(edges, edges, 0, 1, 3);

////////////////////////////////////////////////////////////////////////
        std::vector<cv::Vec4i> lines;
        int min_len_val = 8;
        cv::HoughLinesP(edges, lines, 1, CV_PI / 180, 10, min_len_val, 0);

        curves out;
        dumpbin dump(compAngle);
        splitLines(lines,dump,out,10,8);
        double cap_y = frame.rows*.6;
        for(auto line:out){
//            std::cout << line;
            double cos_theta = line[0];
            double sin_theta = line[1];
            double x0 = line[2], y0 = line[3];
            double tho = atan2(sin_theta, cos_theta) + CV_PI / 2.0;//angle
            double rho = y0 * cos_theta - x0 * sin_theta;
            drawLine(frame,tho, rho, cv::Scalar(0,0,255),cap_y);
//            cv::line(frame,cv::Point(edges.cols-1,righty),cv::Point(0,lefty),cv::Scalar(255,0,0),2);
        }
//        std::cout << std::endl;
        //////////////////////////////////////////////////////////////////////////

        for(auto v:dump){
            auto l = v.first;
            std::cout<< angle(l)<<",";
            cv::line(frame, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(255, 0, 255), 2, cv::LINE_AA);
        }
        //////////////////////////////////////////////////////////////////////////


        std::cout<<std::endl<<std::string(80,'=')<<std::endl;
        cv::line(frame, cv::Point(frame.cols/2, frame.rows), cv::Point(frame.cols/2, 0), cv::Scalar(255, 0, 255), 2, cv::LINE_AA);
        cv::line(frame, cv::Point(0, cap_y), cv::Point(frame.cols, cap_y), cv::Scalar(255, 0, 255), 2, cv::LINE_AA);
//////////////////////////////////////////////////////////////////////////
        int framepos = cap.get(cv::CAP_PROP_POS_FRAMES);

        QDateTime now = QDateTime::currentDateTime();
        QString txt = QString("[%1][%2/%3] %4ms, fps = %5")
                .arg(QTime::fromMSecsSinceStartOfDay(epoch.msecsTo(now)).toString("hh:mm:ss.zzz"))
                .arg(framepos,3).arg(total).arg(last.msecsTo(now),3)
                .arg((framepos-startframe)*1000.0/epoch.msecsTo(now), 0, 'f', 2);
        last = now;

        cv::putText(frame,txt.toStdString(),cv::Point2d(0,30),cv::FONT_HERSHEY_SIMPLEX, .8, cv::Scalar(255,0, 255));
        bwImage = QImage(edges.data, edges.cols, edges.rows, edges.step, QImage::Format_Grayscale8);
        cvtColor(frame, buf,cv::COLOR_BGR2RGB);
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
    int total = cap.get(cv::CAP_PROP_FRAME_COUNT);
//    double pos =3/18.33;
    qDebug()<<pos<< total << cap.set(cv::CAP_PROP_POS_FRAMES, pos*total/100);
}

