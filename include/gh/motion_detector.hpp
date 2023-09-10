//
// Copyright (c) 2023 Gary Huang (ghuang dot nctu at gmail dot com)
//

#ifndef GH_MOTION_DETECTOR_HPP
#define GH_MOTION_DETECTOR_HPP

#include "webcam.hpp"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

namespace gh {

class motion_detector : public webcam_extension
{
public:
    motion_detector()
    : m_blur_ksize(cv::Size(4, 4))
    , m_mark(false)
    , m_debug(false)
    { }

    motion_detector(cv::InputArray frame)
    : m_blur_ksize(cv::Size(4, 4))
    , m_mark(false)
    , m_debug(false)
    { init(frame); }

    motion_detector(const motion_detector&) = delete;
    motion_detector& operator=(const motion_detector&) = delete;

    auto init(cv::InputArray frame) -> void override{
        cv::blur(frame, m_avg, m_blur_ksize);
        m_avg.convertTo(m_avg_float, CV_32F);
    }

    auto mark() -> void
    { m_mark = true; }

    auto debug() -> void
    { m_debug = true; }

    auto update(cv::InputOutputArray frame) -> bool override
    {
        // Source: https://blog.gtwang.org/programming/opencv-motion-detection-and-tracking-tutorial/
        cv::Mat blur;
        cv::blur(frame, blur, m_blur_ksize);

        cv::Mat diff;
        cv::absdiff(m_avg, blur, diff);

        cv::Mat gray;
        cv::cvtColor(diff, gray, cv::COLOR_BGR2GRAY);

        cv::Mat thresh;
        cv::threshold(gray, thresh, 25, 255, cv::THRESH_BINARY);

        cv::Mat kernel = cv::getStructuringElement(0, cv::Size(5, 5));
        cv::morphologyEx(thresh, thresh, cv::MORPH_OPEN, kernel, cv::Point(-1, -1), 2);
        cv::morphologyEx(thresh, thresh, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), 2);

        std::vector<cv::Vec4i> hierarchy;
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(thresh.clone(), contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        bool detected = false;
        for (auto& c : contours) {
            // Ignore small area
            if (cv::contourArea(c) < 2500) {
                continue;
            }
            detected = true;
            if (m_mark) {
                cv::Rect rec = cv::boundingRect(c);
                cv::rectangle(frame, rec, cv::Scalar(0, 255, 0), 2);
            }
        }

        if (m_debug) {
            cv::drawContours(frame, contours, -1, cv::Scalar(0, 255, 255), 2);
        }

        cv::accumulateWeighted(blur, m_avg_float, 0.01);
        cv::convertScaleAbs(m_avg_float, m_avg);
        return detected;
    }

private:
    cv::Mat m_avg;
    cv::Mat m_avg_float;
    cv::Size m_blur_ksize;
    bool m_mark;
    bool m_debug;
};

} // namespace gh

#endif // GH_MOTION_DETECTOR_HPP