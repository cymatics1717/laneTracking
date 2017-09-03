#include "mediasource.h"
#include <QDateTime>
#include <QDebug>
#include <QImage>
#include <QThread>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
mediaSource::mediaSource(QObject *parent) : mediaSource("",parent)
{
//
}

mediaSource::mediaSource(QString name,QObject *parent)
{
    qDebug() << "source = "<<name << setSource(name);
     ld = new LaneDetection;
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

void mediaSource::run()
{
//    bool verbose_lm_detction = false;
//    bool verbose_seed_gen = false;
//    bool verbose_run_crf = false;
//    bool verbose_validating = true;
//    bool verbose = verbose_lm_detction | verbose_seed_gen | verbose_run_crf | verbose_validating;


    static int cnt = 0;
    QDateTime last = QDateTime::currentDateTime();
    epoch = last;

    cv::Mat edges, frame;
    cap.read(frame);

    while(cap.read(frame))
    {
//        if (!ld->initialize_variable(frame)) {
//            return;
//        }

//        if (!ld->initialize_Img(frame)) {
//            continue;
//        }

        // detecting lane markings
//        ld->lane_marking_detection(verbose_lm_detction);

        // supermarking generation and low-level association
//        ld->seed_generation(verbose_seed_gen);

        // CRF graph configuration & optimization using hungarian method
//        ld->graph_generation(verbose_run_crf);

        // validating
//        ld->validating_final_seeds(verbose_validating);


        cv::cvtColor(frame, edges, cv::COLOR_BGR2GRAY);
        cv::GaussianBlur(edges, edges, cv::Size(5,5), 0, 0);
        cv::Canny(edges, edges, 0, 60, 3);
//////////////////////////////////////////////////////////////////////////
        int r_max_val = 70, r_min_val = 30;
        int l_max_val = 180, l_min_val = 110;

        int min_len_val = 40;
        int y_tresh_val = 60;
        int max_x,max_y = frame.rows;
        std::vector<cv::Vec4i> lines;
        cv::HoughLinesP(edges, lines, 1, CV_PI / 180, 50, min_len_val, 6);
        for (size_t i = 0; i < lines.size(); i++)
        {
            cv::Vec4i l = lines[i];
            double theta_deg = atan2(l[2] - l[0], l[3] - l[1]) * 180 / CV_PI;
            if ((r_min_val < theta_deg && theta_deg < r_max_val) || (l_min_val < theta_deg && theta_deg < l_max_val))
            {
                if((l[3]>y_tresh_val * max_y / 100)&&(l[1])>y_tresh_val * max_y / 100)
                    cv::line(frame, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(255, 0, 255), 2, CV_AA);
            }
        }
////////////////////////////////////////////////////////////////////////

        cvtColor(frame, frame,CV_BGR2RGB);

//        current = QImage(edges.data, edges.cols, edges.rows, edges.step, QImage::Format_Grayscale8);
        current = QImage(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888);
        emit incoming();

//////////////////////////////////////////////////////////////////////////
        QDateTime now = QDateTime::currentDateTime();
        qDebug()<<QString("[%1]:%2ms, avg fps = %3 f")
                  .arg(++cnt,3).arg(last.msecsTo(now),3).arg(cnt*1000.0/epoch.msecsTo(now), 0, 'f', 2);
        last = now;
        QThread::msleep(10);
    }

}

void mediaSource::stop()
{
    cap.release();
}

