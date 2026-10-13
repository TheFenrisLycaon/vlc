/******************************
 * Qt player using libHeimdall     *
 * By protonux                *
 *                            *
 * Under WTFPL                *
 ******************************/

#include "player.h"
#include <heimdall/heimdall.h>

#define qtu( i ) ((i).toUtf8().constData())

#include <QtGui>
#include <QMessageBox>
#include <QMenuBar>
#include <QAction>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QUrl>

Mwindow::Mwindow() {
    heimdallPlayer = NULL;

    /* Initialize libHeimdall */
    heimdallInstance = libheimdall_new(0, NULL);

    /* Complain in case of broken installation */
    if (heimdallInstance == NULL) {
        QMessageBox::critical(this, "Qt libHeimdall player", "Could not init libHeimdall");
        exit(1);
    }

    /* Interface initialization */
    initUI();
}

Mwindow::~Mwindow() {
    /* Release libHeimdall instance on quit */
    if (heimdallInstance)
        libheimdall_release(heimdallInstance);
}

void Mwindow::initUI() {

    /* Menu */
    QMenu *fileMenu = menuBar()->addMenu("&File");
    QMenu *editMenu = menuBar()->addMenu("&Edit");

    QAction *Open    = new QAction("&Open", this);
    QAction *Quit    = new QAction("&Quit", this);
    QAction *playAc  = new QAction("&Play/Pause", this);
    QAction *fsAc  = new QAction("&Fullscreen", this);
    QAction *aboutAc = new QAction("&About", this);

    Open->setShortcut(QKeySequence("Ctrl+O"));
    Quit->setShortcut(QKeySequence("Ctrl+Q"));

    fileMenu->addAction(Open);
    fileMenu->addAction(aboutAc);
    fileMenu->addAction(Quit);
    editMenu->addAction(playAc);
    editMenu->addAction(fsAc);

    connect(Open,    SIGNAL(triggered()), this, SLOT(openFile()));
    connect(playAc,  SIGNAL(triggered()), this, SLOT(play()));
    connect(aboutAc, SIGNAL(triggered()), this, SLOT(about()));
    connect(fsAc,    SIGNAL(triggered()), this, SLOT(fullscreen()));
    connect(Quit,    SIGNAL(triggered()), qApp, SLOT(quit()));

    /* Buttons for the UI */
    playBut = new QPushButton("Play");
    QObject::connect(playBut, SIGNAL(clicked()), this, SLOT(play()));

    QPushButton *stopBut = new QPushButton("Stop");
    QObject::connect(stopBut, SIGNAL(clicked()), this, SLOT(stop()));

    QPushButton *muteBut = new QPushButton("Mute");
    QObject::connect(muteBut, SIGNAL(clicked()), this, SLOT(mute()));

    QPushButton *fsBut = new QPushButton("Fullscreen");
    QObject::connect(fsBut, SIGNAL(clicked()), this, SLOT(fullscreen()));

    volumeSlider = new QSlider(Qt::Horizontal);
    QObject::connect(volumeSlider, SIGNAL(sliderMoved(int)), this, SLOT(changeVolume(int)));
    volumeSlider->setValue(80);

    slider = new QSlider(Qt::Horizontal);
    slider->setMaximum(1000);
    QObject::connect(slider, SIGNAL(sliderMoved(int)), this, SLOT(changePosition(int)));

    /* A timer to update the sliders */
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateInterface()));
    timer->start(100);

    /* Central Widgets */
    QWidget* centralWidget = new QWidget;
    videoWidget = new QWidget;

    videoWidget->setAutoFillBackground( true );
    QPalette plt = palette();
    plt.setColor( QPalette::Window, Qt::black );
    videoWidget->setPalette( plt );

    /* Put all in layouts */
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(playBut);
    layout->addWidget(stopBut);
    layout->addWidget(muteBut);
    layout->addWidget(fsBut);
    layout->addWidget(volumeSlider);

    QVBoxLayout *layout2 = new QVBoxLayout;
    layout2->setContentsMargins(0, 0, 0, 0);
    layout2->addWidget(videoWidget);
    layout2->addWidget(slider);
    layout2->addLayout(layout);

    centralWidget->setLayout(layout2);
    setCentralWidget(centralWidget);
    resize( 600, 400);
}

void Mwindow::openFile() {

    /* The basic file-select box */
    QUrl url = QFileDialog::getOpenFileUrl(this, tr("Load a file"));

    /* Stop if something is playing */
    if (heimdallPlayer && libheimdall_media_player_is_playing(heimdallPlayer))
        stop();

    /* Create a new Media */
    libheimdall_media_t *heimdallMedia = libheimdall_media_new_location(qtu(url.toString(QUrl::FullyEncoded)));
    if (!heimdallMedia)
        return;

    /* Create a new libheimdall player */
    heimdallPlayer = libheimdall_media_player_new_from_media (heimdallInstance, heimdallMedia, NULL, NULL);

    /* Release the media */
    libheimdall_media_release(heimdallMedia);

    /* Integrate the video in the interface */
#if defined(Q_OS_MAC)
    libheimdall_media_player_set_nsobject(heimdallPlayer, (void *)videoWidget->winId());
#elif defined(Q_OS_UNIX)
    libheimdall_media_player_set_xwindow(heimdallPlayer, videoWidget->winId());
#elif defined(Q_OS_WIN)
    LONG style = GetWindowLong( videoWidget->winId(), GWL_STYLE );
    if( !(style & WS_CLIPCHILDREN) )
        SetWindowLong( videoWidget->winId(), GWL_STYLE, style | WS_CLIPCHILDREN );
    libheimdall_media_player_set_hwnd(heimdallPlayer, (HWND)videoWidget->winId());
#endif

    /* And start playback */
    libheimdall_media_player_play (heimdallPlayer);

    /* Update playback button */
    playBut->setText("Pause");
}

void Mwindow::play() {
    if (!heimdallPlayer)
        return;

    if (libheimdall_media_player_is_playing(heimdallPlayer))
    {
        /* Pause */
        libheimdall_media_player_pause(heimdallPlayer);
        playBut->setText("Play");
    }
    else
    {
        /* Play again */
        libheimdall_media_player_play(heimdallPlayer);
        playBut->setText("Pause");
    }
}

int Mwindow::changeVolume(int vol) { /* Called on volume slider change */

    if (heimdallPlayer)
        return libheimdall_audio_set_volume (heimdallPlayer,vol);

    return 0;
}

void Mwindow::changePosition(int pos) { /* Called on position slider change */

    if (heimdallPlayer)
        libheimdall_media_player_set_position(heimdallPlayer, (float)pos/1000.0, true);
}

void Mwindow::updateInterface() { //Update interface and check if song is finished

    if (!heimdallPlayer)
        return;

    /* update the timeline */
    float pos = libheimdall_media_player_get_position(heimdallPlayer);
    slider->setValue((int)(pos*1000.0));

    /* Stop the media */
    if (libheimdall_media_player_get_state(heimdallPlayer) == libheimdall_Stopped)
        this->stop();
}

void Mwindow::stop() {
    if(heimdallPlayer) {
        /* stop the media player */
        libheimdall_media_player_stop_async(heimdallPlayer);

        /* release the media player */
        libheimdall_media_player_release(heimdallPlayer);

        /* Reset application values */
        heimdallPlayer = NULL;
        slider->setValue(0);
        playBut->setText("Play");
    }
}

void Mwindow::mute() {
    if(heimdallPlayer) {
        if(volumeSlider->value() == 0) { //if already muted...

                this->changeVolume(80);
                volumeSlider->setValue(80);

        } else { //else mute volume

                this->changeVolume(0);
                volumeSlider->setValue(0);

        }
    }
}

void Mwindow::about()
{
    QMessageBox::about(this, "Qt libHeimdall player demo", QString::fromUtf8(libheimdall_get_version()) );
}

void Mwindow::fullscreen()
{
   if (isFullScreen()) {
       showNormal();
       menuWidget()->show();
   }
   else {
       showFullScreen();
       menuWidget()->hide();
   }
}

void Mwindow::closeEvent(QCloseEvent *event) {
    stop();
    event->accept();
}
