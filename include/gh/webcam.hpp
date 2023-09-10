//
// Copyright (c) 2023 Gary Huang (ghuang dot nctu at gmail dot com)
//

#ifndef GH_WEBCAM_HPP
#define GH_WEBCAM_HPP

#include <exception>

#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

namespace gh {

class webcam_extension
{
public:
    virtual auto init(cv::InputArray frame) -> void = 0;
    virtual auto update(cv::InputOutputArray frame) -> bool = 0;
};

class webcam
{
public:
    webcam()
    : m_cap{}
    { }

    explicit webcam(int index)
    : m_cap{index}
    , m_quality(95)
    {
        if (!m_cap.isOpened()) {
            throw std::system_error(EBUSY, std::generic_category(), "cannot open webcam");
        }
        set_fps(30);
        update();
    }

    webcam(const webcam&) = delete;
    webcam& operator=(const webcam&) = delete;

    auto open(int index) -> void
    {
        m_cap.open(index);
        if (!m_cap.isOpened()) {
            throw std::system_error(EBUSY, std::generic_category(), "cannot open webcam");
        }
        set_fps(30);
        update();
        for (auto ext : m_extensions) {
            ext->init(m_frame);
        }
    }

    auto install(webcam_extension& extension) -> void
    {
        m_extensions.push_back(&extension);
        if (m_cap.isOpened()) {
            extension.init(m_frame);
        }
    }

    auto set_fps(int fps) -> void
    {
        m_cap.set(cv::CAP_PROP_FPS, fps);
    }

    auto set_quality(int quality) -> void
    {
        m_quality = quality;
    }

    auto update() -> void
    {
        if (!m_cap.isOpened()) {
            throw std::system_error(EBUSY, std::generic_category(), "webcam closed");
        }
        m_cap >> m_frame;
        for (auto ext : m_extensions) {
            ext->update(m_frame);
        }
        std::vector<uchar> buf;
        cv::imencode(".jpg", m_frame, buf, std::vector<int>{cv::IMWRITE_JPEG_QUALITY, m_quality});
        if (m_writer.isOpened()) {
            m_writer.write(m_frame);
        }
        boost::unique_lock<boost::shared_mutex> lock(m_mutex);
        m_buffer = std::move(buf);
    }

    auto get() -> std::vector<uchar>
    {
        boost::shared_lock<boost::shared_mutex> lock(m_mutex);
        return m_buffer;
    }

    void record_video(const char* path, double fps = 20.0)
    {
        int codec = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
        if (!m_writer.isOpened()) {
            m_writer.open(path, codec, fps, m_frame.size(), m_frame.type() == CV_8UC3);
        }
        if (!m_writer.isOpened()) {
            throw std::system_error(EBUSY, std::generic_category(), "Cannot open live.avi");
        }
    }

    void stop_record()
    {
        if (m_writer.isOpened()) {
            m_writer.release();
        }
    }

    void take_picture(const char* path)
    {
        cv::imwrite(path, m_frame);
    }

    void run()
    {
        const char* window_name = "Live";

        cv::namedWindow(window_name, cv::WINDOW_NORMAL);

        while (true) {
            m_cap >> m_frame;

            cv::imshow(window_name, m_frame);

            if (m_writer.isOpened()) {
                m_writer.write(m_frame);
            }

            if (cv::waitKey(5) == 'q') {
                break;
            }

            if (cv::getWindowProperty(window_name, cv::WND_PROP_VISIBLE) < 1) {
                break;
            }
        }

        cv::destroyAllWindows();
    }

private:
    cv::VideoCapture m_cap;
    int m_quality;
    cv::VideoWriter m_writer;
    cv::Mat m_frame;
    std::vector<uchar> m_buffer;
    std::vector<webcam_extension*> m_extensions;
    boost::shared_mutex m_mutex;
};

} // namespace gh

#endif // GH_WEBCAM_HPP