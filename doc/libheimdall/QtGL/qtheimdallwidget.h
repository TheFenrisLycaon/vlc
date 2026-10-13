#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)

class QtHeimdallWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    QtHeimdallWidget(QWidget *parent = 0);
    ~QtHeimdallWidget();

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    bool playMedia(const char* url);

signals:
    void contextReady(QOpenGLContext *ctx);

public slots:
    void cleanup();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QOpenGLVertexArrayObject m_vao;
    std::unique_ptr<QOpenGLShaderProgram> m_program;

    std::unique_ptr<class HeimdallVideo> mHeimdall;

    void stop();
    struct libheimdall_instance_t*  m_heimdall = nullptr;
    struct libheimdall_media_player_t* m_mp = nullptr;
    struct libheimdall_media_t* m_media = nullptr;

    QOpenGLBuffer vertexBuffer, vertexIndexBuffer;
};

#endif /* GLWIDGET_H */
