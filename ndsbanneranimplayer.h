#ifndef NDSBANNERANIMPLAYER_H
#define NDSBANNERANIMPLAYER_H

#include "mainwindow.h"

#include <QTimer>
#include <QDialog>
#include <QStatusBar>

namespace Ui {
class NDSBannerAnimPlayer;
}

class NDSBannerAnimPlayer : public QDialog
{
    Q_OBJECT

public:
    explicit NDSBannerAnimPlayer(QWidget *parent, NDSBanner* banner);
    ~NDSBannerAnimPlayer();

protected:
      void showEvent(QShowEvent *ev);

private:
    Ui::NDSBannerAnimPlayer *ui;

    QGraphicsScene graphicsScene;
    NDSBanner* bannerBin;

    QTimer playTimer;
    int durationDelay = 0;
    int currentFrame = 0;
    int currentTick = 0;

    QPixmap frames[64];
    int tickCount = 1;
    int frameCount = 0;

private slots:

    void window_shown();
    void updateProgress();
    void updatePlay();
    void on_play_pb_clicked();

    void stopPlayer();
    void on_frame_sb_valueChanged(int arg1);
};

#endif // NDSBANNERANIMPLAYER_H
