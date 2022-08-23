#include "ndsbanneranimplayer.h"
#include "ui_ndsbanneranimplayer.h"

#include "qndsimage.h"

#include <QPixmap>

NDSBannerAnimPlayer::NDSBannerAnimPlayer(QWidget *parent, NDSBanner* banner) :
    QDialog(parent),
    ui(new Ui::NDSBannerAnimPlayer)
{
    ui->setupUi(this);
    this->setFixedSize(size());

    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    this->bannerBin = banner;

    this->playTimer.setParent(this);
    connect(&this->playTimer, &QTimer::timeout, this, &NDSBannerAnimPlayer::updatePlay);

    ui->graphicsView->scale(4, 4);
    ui->graphicsView->setScene(&this->graphicsScene);

    ui->statusLabel->setText(tr("Rendering, please wait..."));
}

NDSBannerAnimPlayer::~NDSBannerAnimPlayer()
{
    delete ui;
}

void NDSBannerAnimPlayer::showEvent(QShowEvent *ev)
{
    QDialog::showEvent(ev);
    QTimer::singleShot(500, this, &NDSBannerAnimPlayer::window_shown);
}

void NDSBannerAnimPlayer::window_shown()
{
    for(int i = 0; i < 64; i++)
    {
        NDSBanner::AnimSeq* animData = &this->bannerBin->animData[i];
        if(animData->frameDuration)
        {
            u8* pNCG = this->bannerBin->iconExtraNCG[animData->ncgID];
            u16* pNCL = this->bannerBin->iconExtraNCL[animData->nclID];
            QVector<u8> ncg(pNCG, pNCG + 0x200);
            QVector<u16> ncl(pNCL, pNCL + 0x10);

            QNDSImage ndsImg(ncg, ncl, true);
            this->frames[i] = QPixmap::fromImage(ndsImg.toImage(4));

            int xs = animData->flipH ? -1 : 1;
            int ys = animData->flipV ? -1 : 1;
            this->frames[i] = this->frames[i].transformed(QTransform().scale(xs, ys));

            this->frameCount++;
            this->tickCount += animData->frameDuration;
        }
        else
        {
            break;
        }
    }

    this->graphicsScene.addPixmap(this->frames[0]);

    ui->play_pb->setEnabled(true);
    ui->frame_sb->setMaximum(this->frameCount);
    ui->progressBar->setMaximum(this->tickCount);

    updateProgress();
}

void NDSBannerAnimPlayer::updateProgress()
{
    ui->statusLabel->setText(
                tr("Frame ") + QString::number(this->currentFrame + 1) + " / " + QString::number(this->frameCount)
                + "  |  " +
                tr("Tick ") + QString::number(this->currentTick + 1) + " / " + QString::number(this->tickCount));
    ui->progressBar->setValue(this->currentTick + 1);

    ui->frame_sb->setValue(this->currentFrame + 1);
}

void NDSBannerAnimPlayer::updatePlay()
{
    NDSBanner::AnimSeq* animData = &this->bannerBin->animData[this->currentFrame];

    updateProgress();

    if(this->durationDelay == animData->frameDuration)
    {
        this->durationDelay = 0;
        this->currentFrame++;

        if(this->currentFrame == this->frameCount)
        {
            if(ui->loop_cb->isChecked())
            {
                this->currentFrame = 0;
                this->currentTick = 0;
            }
            else
            {
                stopPlayer();
                return;
            }
        }

        this->graphicsScene.clear();
        this->graphicsScene.addPixmap(this->frames[this->currentFrame]);
    }

    this->durationDelay++;
    this->currentTick++;
}

void NDSBannerAnimPlayer::on_play_pb_clicked()
{
    ui->play_pb->setEnabled(false);
    ui->stop_pb->setEnabled(true);
    ui->frame_sb->blockSignals(true);
    ui->frame_sb->setEnabled(false);

    this->durationDelay = 0;
    this->currentFrame = 0;
    this->currentTick = 0;

    this->graphicsScene.clear();
    this->graphicsScene.addPixmap(this->frames[0]);

    this->playTimer.setSingleShot(false);
    this->playTimer.start(17/*1000 / 60*/); // ~60hz

    updateProgress();
}

void NDSBannerAnimPlayer::stopPlayer()
{
    ui->play_pb->setEnabled(true);
    ui->stop_pb->setEnabled(false);
    ui->frame_sb->blockSignals(false);
    ui->frame_sb->setEnabled(true);
    this->playTimer.stop();
}

void NDSBannerAnimPlayer::on_frame_sb_valueChanged(int arg1)
{
    this->currentFrame = arg1 - 1;
    this->currentTick = 0;

    if(this->currentFrame)
        for(int i = 0; i < this->currentFrame; i++)
            this->currentTick += this->bannerBin->animData[i].frameDuration;

    this->graphicsScene.clear();
    this->graphicsScene.addPixmap(this->frames[this->currentFrame]);

    updateProgress();
}
