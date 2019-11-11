#ifndef RENDERTHREAD_H
#define RENDERTHREAD_H

#include <QMutex>
#include <QSize>
#include <QThread>
#include <QWaitCondition>

QT_BEGIN_NAMESPACE
class QImage;
QT_END_NAMESPACE

class RenderThread : public QThread
{
    Q_OBJECT

public:
    RenderThread(QObject *parent = 0);
    ~RenderThread();

    void render(double center_x, double center_y, double scale_factor, QSize result_size);

signals:
    void renderedImage(const QImage &image, double scale_factor);

protected:
    void run() override;

private:
    uint rgbFromWaveLength(double wave);

    QMutex m_mutex;
    QWaitCondition m_condition;
    double m_center_x{0};
    double m_center_y{0};
    double m_scale_factor{0};
    QSize m_result_size;
    bool m_restart{false};
    std::shared_ptr<std::atomic<bool>> m_cancel_token{std::make_shared<std::atomic<bool>>(false)};
    bool m_abort{false};

    enum { ColormapSize = 512 };
    uint m_colormap[ColormapSize];
};

#endif // RENDERTHREAD_H
