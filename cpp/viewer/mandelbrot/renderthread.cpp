#include "renderthread.h"

#include <QtWidgets>
#include <cmath>

#include <fractal/distributed_generator.h>
#include <fractal/fractal_view.h>
#include <fractal/julia_function.h>
#include <fractal/mandlebrot_function.h>
#include <fractal/messages.h>

RenderThread::RenderThread(QObject *parent)
: QThread(parent)
{
    for (int i = 0; i < ColormapSize; ++i)
    {
        m_colormap[i] = rgbFromWaveLength(380.0 + (i * 400.0 / ColormapSize));
    }
}

RenderThread::~RenderThread()
{
    m_mutex.lock();
    m_abort = true;
    m_condition.wakeOne();
    m_mutex.unlock();

    wait();
}

void RenderThread::render(double center_x, double center_y, double scale_factor, QSize result_size)
{
    QMutexLocker locker(&m_mutex);

    m_center_x = center_x;
    m_center_y = center_y;
    m_scale_factor = scale_factor;
    m_result_size = result_size;

    if (!isRunning())
    {
        start(LowPriority);
    }
    else
    {
        m_restart = true;
        *m_cancel_token = true;
        m_condition.wakeOne();
    }
}

void RenderThread::run()
{
    forever {
        auto result_size = QSize{};
        auto scale_factor = double{0.0};
        auto center_x = double{0.0};
        auto center_y = double{0.0};
        {
            QMutexLocker lock{&m_mutex};
            result_size = m_result_size;
            scale_factor = m_scale_factor;
            center_x = m_center_x;
            center_y = m_center_y;
        }

        auto half_width = result_size.width() / 2;
        auto half_height = result_size.height() / 2;

        auto pixel_view = Fractal::View<size_t>{0, 0, static_cast<size_t>(result_size.width()), static_cast<size_t>(result_size.height())};
        auto complex_view = Fractal::View<double>{center_x - (half_width * scale_factor),
                                                  center_y - (half_height * scale_factor),
                                                  center_x + (half_width * scale_factor),
                                                  center_y + (half_height * scale_factor)};
        auto fractal_view = Fractal::Fractal_view{std::move(pixel_view), std::move(complex_view)};

        auto function = Fractal::Mandlebrot_function{1024};
        //auto function = Fractal::Julia_function{{-0.8, 0.156}, 50};

        auto on_task_completed = [](const Fractal::Task_parameters&){};
        auto on_task_canceled = [](const Fractal::Task_parameters&){};
        //auto cancel_token = std::make_shared<std::atomic<bool>>(false);
        auto tasks = Fractal::Generator_task_parameters::distribute("1",
                                                                     m_cancel_token,
                                                                     on_task_completed,
                                                                     on_task_canceled,
                                                                     fractal_view,
                                                                     512,
                                                                     512);

        auto generator = Fractal::Distributed_generator{3};
        generator(function, tasks);

        if (!m_restart)
        {
            auto image = QImage{reinterpret_cast<uchar*>(fractal_view.buffer().get()),
                                static_cast<int>(fractal_view.pixel_view().width()),
                                static_cast<int>(fractal_view.pixel_view().height()),
                                QImage::Format_ARGB32};
            emit renderedImage(image, scale_factor);
        }

        {
            QMutexLocker lock{&m_mutex};
            if (!m_restart)
            {
                m_condition.wait(&m_mutex);
            }
            m_restart = false;
            *m_cancel_token = false;
        }

        if (m_abort)
        {
            return;
        }
    }
}

uint RenderThread::rgbFromWaveLength(double wave)
{
    auto r = double{0.0};
    auto g = double{0.0};
    auto b = double{0.0};

    if (wave >= 380.0 && wave <= 440.0)
    {
        r = -1.0 * (wave - 440.0) / (440.0 - 380.0);
        b = 1.0;
    }
    else if (wave >= 440.0 && wave <= 490.0)
    {
        g = (wave - 440.0) / (490.0 - 440.0);
        b = 1.0;
    }
    else if (wave >= 490.0 && wave <= 510.0)
    {
        g = 1.0;
        b = -1.0 * (wave - 510.0) / (510.0 - 490.0);
    }
    else if (wave >= 510.0 && wave <= 580.0)
    {
        r = (wave - 510.0) / (580.0 - 510.0);
        g = 1.0;
    }
    else if (wave >= 580.0 && wave <= 645.0)
    {
        r = 1.0;
        g = -1.0 * (wave - 645.0) / (645.0 - 580.0);
    }
    else if (wave >= 645.0 && wave <= 780.0)
    {
        r = 1.0;
    }

    auto s = double{1.0};
    if (wave > 700.0)
    {
        s = 0.3 + 0.7 * (780.0 - wave) / (780.0 - 700.0);
    }
    else if (wave <  420.0)
    {
        s = 0.3 + 0.7 * (wave - 380.0) / (420.0 - 380.0);
    }

    r = std::pow(r * s, 0.8);
    g = std::pow(g * s, 0.8);
    b = std::pow(b * s, 0.8);
    return qRgb(int(r * 255), int(g * 255), int(b * 255));
}
