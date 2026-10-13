/******************************
 * Qt player using libHeimdall     *
 * By protonux                *
 *                            *
 * Under WTFPL                *
 ******************************/

#ifndef PLAYER
#define PLAYER

#include <QtGui>
#include <QMainWindow>
#include <QPushButton>
#include <QSlider>
#include <QWidget>
#include <heimdall/heimdall.h>

class Mwindow : public QMainWindow {

    Q_OBJECT

        public:
               Mwindow();
               virtual ~Mwindow();

        private slots:
               void openFile();
               void play();
               void stop();
               void mute();
               void about();
               void fullscreen();

               int changeVolume(int);
               void changePosition(int);
               void updateInterface();

        protected:
               virtual void closeEvent(QCloseEvent*);

        private:
               QPushButton *playBut;
               QSlider *volumeSlider;
               QSlider *slider;
               QWidget *videoWidget;

               libheimdall_instance_t *heimdallInstance;
               libheimdall_media_player_t *heimdallPlayer;

               void initUI();
};


#endif
