#include "qtheimdallwidget.h"
#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QCoreApplication>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QOffscreenSurface>
#include <QThread>
#include <QSemaphore>
#include <QMutex>
#include <cmath>

#include <heimdall/heimdall.h>

class HeimdallVideo
{
public:
    HeimdallVideo(QtHeimdallWidget *widget)
        :mWidget(widget)
    {
        /* Use default format for context. */
        mContext = new QOpenGLContext(widget);

        /* Use offscreen surface to render the buffers */
        mSurface = new QOffscreenSurface(nullptr, widget);

        /* Widget doesn't have an OpenGL context right now, we'll get it later. */
        QObject::connect(widget, &QtHeimdallWidget::contextReady, [this](QOpenGLContext *render_context) {
            /* Video view is now ready, we can start */
            mSurface->setFormat(mWidget->format());
            mSurface->create();

            mContext->setFormat(mWidget->format());
            mContext->setShareContext(render_context);
            mContext->create();

            videoReady.release();
        });
    }

    ~HeimdallVideo()
    {
        cleanup(this);
    }

    /// return whether we are using OpenGLES or OpenGL, which is needed to know
    /// whether we use the libheimdall OpenGLES2 engine or the OpenGL one
    bool isOpenGLES()
    {
        return mContext->isOpenGLES();
    }

    /// return the texture to be displayed
    QOpenGLFramebufferObject *getVideoFrame()
    {
        QMutexLocker locker(&m_text_lock);
        if (m_updated) {
            std::swap(m_idx_swap, m_idx_display);
            m_updated = false;
        }
        return mBuffers[m_idx_display].get();
    }

    void onResize(QSize size)
    {
        QMutexLocker lock(&m_cb_lock);
        m_windowSize = size;
        if (!report_size_change)
            return;
        report_size_change(reportOpaque, size.width(), size.height());
    }

    void onMouseMove(QPoint pos)
    {
        QMutexLocker lock(&m_cb_lock);
        if (!report_mouse_move)
            return;
        report_mouse_move(reportOpaque, pos.x(), pos.y());
    }

    void onMousePress(Qt::MouseButton button)
    {
        QMutexLocker lock(&m_cb_lock);
        if (!report_mouse_press)
            return;
        bool ok;
        auto heimdallButton = qtToHeimdallMouseButton(button, &ok);
        if (!ok)
            return;
        report_mouse_press(reportOpaque, heimdallButton);
    }

    void onMouseRelease(Qt::MouseButton button)
    {
        QMutexLocker lock(&m_cb_lock);
        if (!report_mouse_release)
            return;
        bool ok;
        auto heimdallButton = qtToHeimdallMouseButton(button, &ok);
        if (!ok)
            return;
        report_mouse_release(reportOpaque, heimdallButton);
    }

    static void setReportCallbacks(void* data,
        libheimdall_video_output_resize_cb report_size_change,
        libheimdall_video_output_mouse_move_cb report_mouse_move,
        libheimdall_video_output_mouse_press_cb report_mouse_press,
        libheimdall_video_output_mouse_release_cb report_mouse_release,
        void* reportOpaque)
    {
        HeimdallVideo* that = static_cast<HeimdallVideo*>(data);
        QMutexLocker lock(&that->m_cb_lock);
        that->reportOpaque = reportOpaque;
        that->report_size_change = report_size_change;
        that->report_mouse_move = report_mouse_move;
        that->report_mouse_press = report_mouse_press;
        that->report_mouse_release = report_mouse_release;

        //report initial window size
        if (report_size_change && that->m_windowSize.isValid())
            report_size_change(that->reportOpaque, that->m_windowSize.width(), that->m_windowSize.height());
    }

    /// this callback will create the surfaces and FBO used by Heimdall to perform its rendering
    static bool resizeRenderTextures(void* data, const libheimdall_video_render_cfg_t *cfg,
                                     libheimdall_video_output_cfg_t *render_cfg)
    {
        HeimdallVideo* that = static_cast<HeimdallVideo*>(data);
        if (cfg->width != that->m_width || cfg->height != that->m_height)
            cleanup(data);

        for (auto &buffer : that->mBuffers)
            buffer = std::make_unique<QOpenGLFramebufferObject>(cfg->width, cfg->height);

        that->m_width = cfg->width;
        that->m_height = cfg->height;

        that->mBuffers[that->m_idx_render]->bind();

        render_cfg->u.opengl_format = GL_RGBA;
        render_cfg->full_range = true;
        render_cfg->colorspace = libheimdall_video_colorspace_BT709;
        render_cfg->primaries  = libheimdall_video_primaries_BT709;
        render_cfg->transfer   = libheimdall_video_transfer_func_SRGB;
        render_cfg->orientation = libheimdall_video_orient_top_left;

        return true;
    }

    // This callback is called during initialisation.
    static bool setup(void** data, const libheimdall_video_setup_device_cfg_t *cfg,
                      libheimdall_video_setup_device_info_t *out)
    {
        Q_UNUSED(cfg);
        Q_UNUSED(out);

        if (!QOpenGLContext::supportsThreadedOpenGL())
            return false;

        HeimdallVideo* that = static_cast<HeimdallVideo*>(*data);

        /* Wait for rendering view to be ready. */
        that->videoReady.acquire();

        that->m_width = 0;
        that->m_height = 0;
        return true;
    }


    // This callback is called to release the texture and FBO created in resize
    static void cleanup(void* data)
    {
        HeimdallVideo* that = static_cast<HeimdallVideo*>(data);

        that->videoReady.release();

        if (that->m_width == 0 && that->m_height == 0)
            return;
        for (auto &buffer : that->mBuffers)
            buffer.reset(nullptr);
    }

    //This callback is called after Heimdall performs drawing calls
    static void swap(void* data)
    {
        HeimdallVideo* that = static_cast<HeimdallVideo*>(data);
        QMutexLocker locker(&that->m_text_lock);
        that->m_updated = true;
        that->mWidget->update();
        std::swap(that->m_idx_swap, that->m_idx_render);
        that->mBuffers[that->m_idx_render]->bind();
    }

    // This callback is called to set the OpenGL context
    static bool make_current(void* data, bool current)
    {
        HeimdallVideo* that = static_cast<HeimdallVideo*>(data);
        if (current)
            that->mContext->makeCurrent(that->mSurface);
        else
            that->mContext->doneCurrent();
        return true;
    }

    // This callback is called by Heimdall to get OpenGL functions.
    static void* get_proc_address(void* data, const char* current)
    {
        HeimdallVideo* that = static_cast<HeimdallVideo*>(data);
        /* Qt usual expects core symbols to be queryable, even though it's not
         * mentioned in the API. Cf QPlatformOpenGLContext::getProcAddress.
         * Thus, we don't need to wrap the function symbols here, but it might
         * fail to work (not crash though) on exotic platforms since Qt doesn't
         * commit to this behaviour. A way to handle this in a more stable way
         * would be to store the mContext->functions() table in a thread local
         * variable, and wrap every call to OpenGL in a function using this
         * thread local state to call the correct variant. */
        return reinterpret_cast<void*>(that->mContext->getProcAddress(current));
    }

private:
    libheimdall_video_output_mouse_button_t qtToHeimdallMouseButton(Qt::MouseButton button, bool* ok)
    {
        assert(ok != nullptr);
        *ok = true;
        switch (button)
        {
        case Qt::LeftButton:
            return libheimdall_video_output_mouse_button_left;
        case Qt::RightButton:
            return libheimdall_video_output_mouse_button_right;
        case Qt::MiddleButton:
            return libheimdall_video_output_mouse_button_middle;
        default:
            *ok = false;
            return libheimdall_video_output_mouse_button_left;
        }
    }

    QtHeimdallWidget *mWidget;
    QOpenGLContext *mContext;
    QOffscreenSurface *mSurface;
    QSemaphore videoReady;

    //FBO data
    unsigned m_width = 0;
    unsigned m_height = 0;
    QMutex m_text_lock;
    std::unique_ptr<QOpenGLFramebufferObject> mBuffers[3];
    size_t m_idx_render = 0;
    size_t m_idx_swap = 1;
    size_t m_idx_display = 2;
    bool m_updated = false;

    QSize m_windowSize;

    QMutex m_cb_lock;
    void* reportOpaque = nullptr;
    libheimdall_video_output_resize_cb report_size_change = nullptr;
    libheimdall_video_output_mouse_move_cb report_mouse_move = nullptr;
    libheimdall_video_output_mouse_press_cb report_mouse_press = nullptr;
    libheimdall_video_output_mouse_release_cb report_mouse_release = nullptr;

};


QtHeimdallWidget::QtHeimdallWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      vertexBuffer(QOpenGLBuffer::VertexBuffer),
      vertexIndexBuffer(QOpenGLBuffer::IndexBuffer)
{
    setMouseTracking(true);

    // --transparent causes the clear color to be transparent. Therefore, on systems that
    // support it, the widget will become transparent apart from the logo.

    const char *args[] = {
        "--verbose=2"
    };
    m_heimdall = libheimdall_new(sizeof(args) / sizeof(*args), args);

    mHeimdall = std::make_unique<HeimdallVideo>(this);
}

bool QtHeimdallWidget::playMedia(const char* url)
{
    m_media = libheimdall_media_new_location(url);
    if (m_media == nullptr) {
        fprintf(stderr, "unable to create media %s", url);
        return false;
    }
    m_mp = libheimdall_media_player_new_from_media (m_heimdall, m_media, NULL, NULL);
    if (m_mp == nullptr) {
        fprintf(stderr, "unable to create media player");
        libheimdall_media_release(m_media);
        return false;
    }

    // Define the opengl rendering callbacks
    libheimdall_video_set_output_callbacks(m_mp,
        mHeimdall->isOpenGLES() ? libheimdall_video_engine_gles2 : libheimdall_video_engine_opengl,
        HeimdallVideo::setup, HeimdallVideo::cleanup, HeimdallVideo::setReportCallbacks, HeimdallVideo::resizeRenderTextures, HeimdallVideo::swap,
        HeimdallVideo::make_current, HeimdallVideo::get_proc_address, nullptr, nullptr,
        mHeimdall.get());

    // Play the video
    libheimdall_media_player_play (m_mp);

    return true;
}

QtHeimdallWidget::~QtHeimdallWidget()
{
    cleanup();
}

QSize QtHeimdallWidget::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize QtHeimdallWidget::sizeHint() const
{
    return QSize(400, 400);
}

void QtHeimdallWidget::cleanup()
{
    stop();

    if (m_heimdall)
        libheimdall_release(m_heimdall);
    m_heimdall = nullptr;

    if (m_program == nullptr)
        return;

    makeCurrent();
    vertexBuffer.destroy();
    vertexIndexBuffer.destroy();
    m_program.reset(nullptr);
    doneCurrent();
}

void QtHeimdallWidget::stop()
{
    if (m_mp) {
        libheimdall_media_player_release(m_mp);
        m_mp = nullptr;
    }
    if (m_media) {
        libheimdall_media_release(m_media);
        m_media = nullptr;
    }
}

static const char *vertexShaderSource =
    "attribute vec2 position;\n"
    "varying vec2 texcoord;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = vec4(position, 0.0, 1.0);\n"
    "    texcoord = position * vec2(0.5) + vec2(0.5);\n"
    "}\n";

static const char *fragmentShaderSource =
    /* Precision is available but no-op on GLSL 130 (see [3], section 4.5.2)
     * and mandatory for OpenGL ES. It was first reserved on GLSL 120 and
     * didn't exist in GLSL <= 110 (= OpenGL 2.0). Since it's a no-op, the
     * easiest solution is to never use it for OpenGL code.
     *
     * [1]: https://www.khronos.org/registry/OpenGL/specs/gl/GLSLangSpec.1.10.pdf
     * [2]: https://www.khronos.org/registry/OpenGL/specs/gl/GLSLangSpec.1.20.pdf
     * [3]: https://www.khronos.org/registry/OpenGL/specs/gl/GLSLangSpec.1.30.pdf
     */
    "#ifdef GL_ES\n"
    "precision highp float;\n"
    "#endif\n"

    "\n"
    "uniform sampler2D texture;\n"
    "\n"
    "varying vec2 texcoord;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    gl_FragColor = texture2D(texture, texcoord);\n"
    "}\n";

/*
 * Data used to seed our vertex array and element array buffers:
 */
static const GLfloat g_vertex_buffer_data[] = {
    -1.0f, -1.0f,
     1.0f, -1.0f,
    -1.0f,  1.0f,
     1.0f,  1.0f
};
static const GLushort g_element_buffer_data[] = { 0, 1, 2, 3 };

void QtHeimdallWidget::initializeGL()
{
    // In this example the widget's corresponding top-level window can change
    // several times during the widget's lifetime. Whenever this happens, the
    // QOpenGLWidget's associated context is destroyed and a new one is created.
    // Therefore we have to be prepared to clean up the resources on the
    // aboutToBeDestroyed() signal, instead of the destructor. The emission of
    // the signal will be followed by an invocation of initializeGL() where we
    // can recreate all resources.
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &QtHeimdallWidget::cleanup);

    vertexBuffer.create();
    vertexBuffer.bind();
    vertexBuffer.allocate(g_vertex_buffer_data, sizeof(g_vertex_buffer_data));

    vertexIndexBuffer.create();
    vertexIndexBuffer.bind();
    vertexIndexBuffer.allocate(g_element_buffer_data, sizeof(g_element_buffer_data));

    m_program = std::make_unique<QOpenGLShaderProgram>();
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    m_program->link();

    m_program->setUniformValue("texture", 0);

    m_program->bindAttributeLocation("position", 0);

    emit contextReady(context());
}

void QtHeimdallWidget::paintGL()
{
    QOpenGLFunctions *GL = context()->functions();
    QOpenGLFramebufferObject *fbo = mHeimdall->getVideoFrame();
    if (fbo != nullptr && GL != nullptr)
    {
        m_program->bind();

        GL->glClearColor(1.0, 0.5, 0.0, 1.0);

        GL->glActiveTexture(GL_TEXTURE0);
        GL->glBindTexture(GL_TEXTURE_2D, fbo->texture());

        vertexBuffer.bind();
        m_program->setAttributeArray("position", (const QVector2D *)nullptr, sizeof(GLfloat)*2);

        m_program->enableAttributeArray("position");

        vertexIndexBuffer.bind();
        GL->glDrawElements(
            GL_TRIANGLE_STRIP,  /* mode */
            4,                  /* count */
            GL_UNSIGNED_SHORT,  /* type */
            (void*)0            /* element array buffer offset */
        );

        m_program->disableAttributeArray("position");
    }
}

void QtHeimdallWidget::resizeGL(int w, int h)
{
    mHeimdall->onResize(QSize(w, h));
}

void QtHeimdallWidget::mouseMoveEvent(QMouseEvent *event)
{
    QOpenGLWidget::mouseMoveEvent(event);
    mHeimdall->onMouseMove(event->pos());
}

void QtHeimdallWidget::mousePressEvent(QMouseEvent *event)
{
    QOpenGLWidget::mousePressEvent(event);
    mHeimdall->onMousePress(event->button());
}

void QtHeimdallWidget::mouseReleaseEvent(QMouseEvent *event)
{
    QOpenGLWidget::mouseReleaseEvent(event);
    mHeimdall->onMouseRelease(event->button());
}
