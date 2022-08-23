#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "qndsimage.h"
#include "crc.h"
#include "ndsbanneranimplayer.h"

#include <QVector>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QStandardItemModel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->gfx_gv->scale(4, 4);
    ui->gfx_gv->setScene(&this->gfx_scene);
    this->lastDirPath = QDir::homePath();

    setProgramState(ProgramState::Closed);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setProgramState(ProgramState mode)
{
    bool closed = mode == ProgramState::Closed;

    ui->gfx_gb->setEnabled(!closed);
    ui->gameTitle_gb->setEnabled(!closed);
    ui->bannerVersion_gb->setEnabled(!closed);
    ui->anim_gb->setEnabled(!closed);

    ui->actionNew->setEnabled(closed);
    ui->actionOpen->setEnabled(closed);
    ui->actionSave->setEnabled(mode == ProgramState::KnowsPath);
    ui->actionSave_As->setEnabled(!closed);
    ui->actionClose->setEnabled(!closed);

    ui->gfxBmp_sb->blockSignals(closed);
    ui->gfxPal_sb->blockSignals(closed);
    ui->gameTitle_cb->blockSignals(closed);
    ui->gameTitle_pte->blockSignals(closed);
    ui->bannerVersion_cb->blockSignals(closed);

    setAnimGroupBlockSignals(closed);

    if(closed)
    {
        this->gfx_scene.clear();
        this->openedFileName.clear();

        ui->gfxBmp_sb->setValue(0);
        ui->gfxPal_sb->setValue(0);
        ui->gfxPal_sb->setMinimum(0);

        ui->gameTitle_cb->setCurrentIndex(0);
        ui->gameTitle_pte->setPlainText("");

        ui->bannerVersion_cb->setCurrentIndex(3);

        ui->animFrame_cb->clear();
        setAnimGroupEnabled(false);

        this->gfxBmp_lastValue = 0;
        this->animFrame_lastSize = 0;
    }

    ui->gfxPal_sb->setEnabled(false);

}

void MainWindow::getBinaryIconPtr(u8*& ncg, u16*& ncl, int bmpID, int palID)
{
    if (bmpID == -1)
    {
        ncg = this->bannerBin.iconNCG;
        ncl = this->bannerBin.iconNCL;
    }
    else
    {
        ncg = this->bannerBin.iconExtraNCG[bmpID];
        ncl = this->bannerBin.iconExtraNCL[palID];
    }
}

QImage MainWindow::getCurrentImage(int bmpID, int palID)
{
    u8* ncg;
    u16* ncl;
    getBinaryIconPtr(ncg, ncl, bmpID, palID);

    QVector<u8> ncgV = QVector<u8>(ncg, ncg + 0x200);
    QVector<u16> nclV = QVector<u16>(ncl, ncl + 0x10);

    QNDSImage ndsImg(ncgV, nclV, true);
    return ndsImg.toImage(4);
}

QPixmap MainWindow::getCurrentPixmap(int bmpID, int palID)
{
    return QPixmap::fromImage(getCurrentImage(bmpID, palID));
}

void MainWindow::updateIconView(int bmpID, int palID)
{
    QPixmap pixmap = getCurrentPixmap(bmpID, palID);

    gfx_scene.clear();
    gfx_scene.addPixmap(pixmap);
}

bool MainWindow::checkIfAllowClose()
{
    //If possible, check if file was modified
    QFile openFile(this->openedFileName);
    if(openFile.open(QIODevice::ReadOnly))
    {
        int bannerSize;
        if(this->bannerBin.version & (1 << 8))
            bannerSize = sizeof(NDSBanner);
        else if((this->bannerBin.version & 3) == 3)
            bannerSize = 0xA40;
        else if(this->bannerBin.version & 2)
            bannerSize = 0x940;
        else
            bannerSize = 0x840;

        bool isDifferent = memcmp(openFile.readAll().data(), &this->bannerBin, bannerSize);
        openFile.close();

        if(isDifferent)
        {
            QMessageBox::StandardButton btn = QMessageBox::question(this, tr("You sure?"), tr("There are unsaved changes!\nAre you sure you want to close?"));
            if(btn == QMessageBox::No)
                return false;
        }
    }
    return true;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    checkIfAllowClose() ? event->accept() : event->ignore();
}

/* ======== MENU BAR ACTIONS GROUP ======== */

void MainWindow::on_actionNew_triggered()
{
    memset(&this->bannerBin, 0, sizeof(NDSBanner));

    this->bannerBin.version = 0x0103;

    QFile fileNCG(":/resources/icon_ncg.bin");
    fileNCG.open(QIODevice::ReadOnly);

    QFile fileNCL(":/resources/icon_ncl.bin");
    fileNCL.open(QIODevice::ReadOnly);

    memcpy(this->bannerBin.iconNCG, fileNCG.readAll().data(), 0x200);
    memcpy(this->bannerBin.iconNCL, fileNCL.readAll().data(), 0x20);

    fileNCG.close();
    fileNCL.close();

    QString title = "The crazy smiley quest!\nSmiley Corporation";
    for(int i = 0; i < 8; i++)
        memcpy(&this->bannerBin.title[i], title.data(), title.length() * 2);

    updateIconView(-1, -1);
    on_gameTitle_cb_currentIndexChanged(0);

    setProgramState(ProgramState::NewFile);
}

void MainWindow::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, "", this->lastDirPath, tr("Banner Files") + " (*.bin)");
    if(fileName == "")
        return;
    this->openedFileName = fileName;

    QFileInfo fileInfo(fileName);
    this->lastDirPath = fileInfo.dir().path();

    if(fileInfo.size() != sizeof(NDSBanner) && fileInfo.size() != 0x840 && fileInfo.size() != 0x940 && fileInfo.size() != 0xA40)
    {
        QMessageBox::information(this, tr("Oops!"), tr("Invalid banner size.\nMake sure this is a valid banner file."));
        return;
    }

    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(this, "Big OOF", tr("Could not open file for reading."));
        return;
    }

    memset(&this->bannerBin, 0, sizeof(NDSBanner));
    memcpy(&this->bannerBin, file.readAll().data(), file.size());
    file.close();

    /* ======== ICON SETUP ======== */

    updateIconView(-1, 0);

    /* ======== TEXT SETUP ======== */

    on_gameTitle_cb_currentIndexChanged(0);

    /* ======== VERSION SETUP ======== */

    if(this->bannerBin.version & (1 << 8))
        ui->bannerVersion_cb->setCurrentIndex(3);
    else if((this->bannerBin.version & 3) == 3)
        ui->bannerVersion_cb->setCurrentIndex(2);
    else if(this->bannerBin.version & 2)
        ui->bannerVersion_cb->setCurrentIndex(1);
    else
        ui->bannerVersion_cb->setCurrentIndex(0);

    /* ======== ANIMATION SETUP ======== */

    for(int i = 0; i < 64; i++)
    {
        NDSBanner::AnimSeq* animData = &this->bannerBin.animData[i];
        if(animData->frameDuration)
            ui->animFrame_cb->addItem(tr("Frame ") + QString::number(i + 1));
        else
            break;
    }
    if(this->bannerBin.animData[0].frameDuration)
        setAnimGroupEnabled(true);
    on_animFrame_cb_currentIndexChanged(0);

    setProgramState(ProgramState::KnowsPath);
}

void MainWindow::saveFile(const QString& path)
{
    QFile file(path);
    if(!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(this, "Big OOF", tr("Could not open file for writing."));
        return;
    }

    u8* u16BannerBin = reinterpret_cast<u8*>(&this->bannerBin);

    this->bannerBin.crc[0] = crc16(&u16BannerBin[0x20], 0x840 - 0x20);
    this->bannerBin.crc[1] = (this->bannerBin.version & 2) ? crc16(&u16BannerBin[0x20], 0x940 - 0x20) : 0;
    this->bannerBin.crc[2] = ((this->bannerBin.version & 3) == 3) ? crc16(&u16BannerBin[0x20], 0xA40 - 0x20) : 0;
    this->bannerBin.crc[3] = (this->bannerBin.version & (1 << 8)) ? crc16(&u16BannerBin[0x1240], 0x23C0 - 0x1240) : 0;

    int bannerSize;
    if(this->bannerBin.version & (1 << 8))
        bannerSize = sizeof(NDSBanner);
    else if((this->bannerBin.version & 3) == 3)
        bannerSize = 0xA40;
    else if(this->bannerBin.version & 2)
        bannerSize = 0x940;
    else
        bannerSize = 0x840;

    QByteArray out(bannerSize, 0);
    memcpy(out.data(), &this->bannerBin, bannerSize);

    file.write(out);
    file.close();
}

void MainWindow::on_actionSave_triggered() {
    saveFile(this->openedFileName);
}

void MainWindow::on_actionSave_As_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(this, "", this->lastDirPath, tr("Banner Files") + " (*.bin)");
    if(fileName == "")
        return;
    this->openedFileName = fileName;
    QFileInfo fileInfo(fileName);
    this->lastDirPath = fileInfo.dir().path();

    setProgramState(ProgramState::KnowsPath);

    saveFile(fileName);
}

void MainWindow::on_actionClose_triggered()
{
    if(!checkIfAllowClose())
        return;

    //Clear everything
    setProgramState(ProgramState::Closed);
}

void MainWindow::on_actionCredits_triggered()
{
    QMessageBox::about(this, tr("About"), R"(<p><strong>Nintendo DS Banner Editor</strong></p>
<p>Copyright &copy; 2020 TheGameratorT</p>
<p><span style="text-decoration: underline;">Special thanks:</span></p>
<p style="padding-left: 30px;">Banner format research by <a href="https://problemkaputt.de/gbatek-ds-cartridge-icon-title.htm">GBATEK<br /></a>Image conversion by <a href="https://github.com/Ed-1T">Ed_IT</a></p>
<p><span style="text-decoration: underline;">License:</span></p>
<p style="padding-left: 30px;">This application is licensed under the GNU General Public License v3.</p>
<p style="padding-left: 30px;">This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.</p>
<p style="padding-left: 30px;">This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.<br />See the GNU General Public License for more details.</p>
<p style="padding-left: 30px;">For details read the LICENSE file bundled with the program or visit:</p>
<p style="padding-left: 30px;"><a href="https://www.gnu.org/licenses/">https://www.gnu.org/licenses/</a></p>)");
}

void MainWindow::on_actionQt_triggered()
{
    QMessageBox::aboutQt(this);
}

/* ======== ICON/GRAPHICS GROUP ======== */

void MainWindow::on_gfxBmp_sb_valueChanged(int arg1)
{
    if(arg1 == 0)
    {
        ui->gfxPal_sb->blockSignals(true);
        ui->gfxPal_sb->setMinimum(0);
        ui->gfxPal_sb->setValue(0);
        ui->gfxPal_sb->setEnabled(false);
    }
    else if (this->gfxBmp_lastValue == 0)
    {
        ui->gfxPal_sb->blockSignals(false);
        ui->gfxPal_sb->setMinimum(1);
        ui->gfxPal_sb->setEnabled(true);
    }

    int bmpID = arg1 - 1;
    int palID = ui->gfxPal_sb->value() - 1;
    updateIconView(bmpID, palID);

    this->gfxBmp_lastValue = arg1;
}

void MainWindow::on_gfxPal_sb_valueChanged(int arg1)
{
    int bmpID = ui->gfxBmp_sb->value() - 1;
    int palID = arg1 - 1;
    updateIconView(bmpID, palID);
}

/* ======== GAME TITLE GROUP ======== */

void MainWindow::on_gameTitle_pte_textChanged()
{
    QString txt = ui->gameTitle_pte->toPlainText();
    if(txt.length() > 0x80)
    {
        txt.resize(0x80);
        ui->gameTitle_pte->setPlainText(txt);
    }

    int current = ui->gameTitle_cb->currentIndex();

    int bytes = txt.length() * 2;
    int chars = bytes / 2;
    memcpy(this->bannerBin.title[current], txt.data(), bytes);
    memset(&this->bannerBin.title[current][chars], 0, 0x100 - bytes);
}

void MainWindow::on_gameTitle_cb_currentIndexChanged(int index) {
    ui->gameTitle_pte->setPlainText(QString(this->bannerBin.title[index]));
}

void MainWindow::on_gameTitle_pb_clicked()
{
    QMessageBox::StandardButton btn = QMessageBox::question(this, tr("You sure?"), tr("Do you really want to replace all language titles with the current one?"));
    if(btn == QMessageBox::No)
        return;

    int current = ui->gameTitle_cb->currentIndex();

    for(int i = 0; i < 8; i++)
    {
        if(i == current)
            continue;

        memcpy(this->bannerBin.title[i], this->bannerBin.title[current], 0x100);
    }
}

/* ======== BANNER VERSION GROUP ======== */

void MainWindow::on_bannerVersion_cb_currentIndexChanged(int index)
{
    constexpr int bannerVersions[] = {
        0x0001, // Normal
        0x0002, // Chinese
        0x0003, // Korean
        0x0103  // DSi
    };

    this->bannerBin.version = bannerVersions[index];

    // Block animation settings if not DSi banner
    bool dsiBanner = this->bannerBin.version & (1 << 8);
    ui->anim_gb->setEnabled(dsiBanner);

    // Only banner 0 exists for non-DSi banners
    if(!dsiBanner)
        ui->gfxBmp_sb->setValue(0);
    ui->gfxBmp_sb->setEnabled(dsiBanner);

    // Ensure the the selected language is valid for this version
    int maxLanguage;
    if((this->bannerBin.version & 3) == 3)
        maxLanguage = 7;
    else if((this->bannerBin.version & 2))
        maxLanguage = 6;
    else
        maxLanguage = 5;

    if(ui->gameTitle_cb->currentIndex() > maxLanguage) {
        ui->gameTitle_cb->setCurrentIndex(0);
    }
    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(ui->gameTitle_cb->model());
    Q_ASSERT(model != nullptr);
    for(int i = 6; i < 8; i++)
        model->item(i)->setEnabled(i <= maxLanguage);
}

/* ======== ANIMATION GROUP ======== */

void MainWindow::setAnimGroupBlockSignals(bool flag)
{
    ui->animFrame_cb->blockSignals(flag);
    ui->animDur_sb->blockSignals(flag);
    ui->animBmp_sb->blockSignals(flag);
    ui->animPal_sb->blockSignals(flag);
    ui->animFlipX_cb->blockSignals(flag);
    ui->animFlipY_cb->blockSignals(flag);
}

void MainWindow::setAnimGroupEnabled(bool flag)
{
    ui->animFrame_cb->setEnabled(flag);
    ui->animFrameRem_pb->setEnabled(flag);

    ui->animDur_label->setEnabled(flag);
    ui->animBmp_label->setEnabled(flag);
    ui->animPal_label->setEnabled(flag);
    ui->animFlip_label->setEnabled(flag);

    int val = flag ? 1 : 0;
    ui->animDur_sb->setMinimum(val);
    ui->animBmp_sb->setMinimum(val);
    ui->animPal_sb->setMinimum(val);

    if(!flag)
    {
        ui->animDur_sb->setValue(0);
        ui->animBmp_sb->setValue(0);
        ui->animPal_sb->setValue(0);
        ui->animFlipX_cb->setChecked(false);
        ui->animFlipY_cb->setChecked(false);
    }

    ui->animDur_sb->setEnabled(flag);
    ui->animBmp_sb->setEnabled(flag);
    ui->animPal_sb->setEnabled(flag);
    ui->animFlipX_cb->setEnabled(flag);
    ui->animFlipY_cb->setEnabled(flag);
}

void MainWindow::on_animFrame_cb_currentIndexChanged(int index)
{
    setAnimGroupBlockSignals(true);

    NDSBanner::AnimSeq* animData = &this->bannerBin.animData[index];
    ui->animDur_sb->setValue(animData->frameDuration);
    ui->animBmp_sb->setValue(animData->ncgID + 1);
    ui->animPal_sb->setValue(animData->nclID + 1);
    ui->animFlipX_cb->setChecked(animData->flipH);
    ui->animFlipY_cb->setChecked(animData->flipV);

    setAnimGroupBlockSignals(false);
}

void MainWindow::on_animFrameAdd_pb_clicked()
{
    int index = ui->animFrame_cb->count();
    if(index != 64)
    {
        ui->animFrame_cb->addItem(tr("Frame ") + QString::number(index + 1));

        NDSBanner::AnimSeq* animData = &this->bannerBin.animData[index];
        animData->frameDuration = 1;

        if(this->animFrame_lastSize == 0)
        {
            setAnimGroupBlockSignals(true);
            setAnimGroupEnabled(true);
            setAnimGroupBlockSignals(false);
        }

        this->animFrame_lastSize = index + 1;
    }
}

void MainWindow::on_animFrameRem_pb_clicked()
{
    int index = ui->animFrame_cb->currentIndex();
    ui->animFrame_cb->removeItem(index);

    int new_count = ui->animFrame_cb->count();
    if(new_count != 0)
    {
        for(int i = index; i < new_count; i++)
        {
            ui->animFrame_cb->setItemText(i, tr("Frame ") + QString::number(i + 1));
            this->bannerBin.animData[i] = this->bannerBin.animData[i + 1];
        }

        int offset = index == new_count ? 1 : 0;
        on_animFrame_cb_currentIndexChanged(index - offset);
    }
    else
    {
        setAnimGroupBlockSignals(true);
        setAnimGroupEnabled(false);
        setAnimGroupBlockSignals(false);
    }

    *reinterpret_cast<u16*>(&this->bannerBin.animData[new_count]) = 0; //Clear last entry
    this->animFrame_lastSize = new_count;
}

void MainWindow::on_animDur_sb_valueChanged(int arg1)
{
    int index = ui->animFrame_cb->currentIndex();
    this->bannerBin.animData[index].frameDuration = arg1;
}

void MainWindow::on_animBmp_sb_valueChanged(int arg1)
{
    int index = ui->animFrame_cb->currentIndex();
    this->bannerBin.animData[index].ncgID = arg1 - 1;
}

void MainWindow::on_animPal_sb_valueChanged(int arg1)
{
    int index = ui->animFrame_cb->currentIndex();
    this->bannerBin.animData[index].nclID = arg1 - 1;
}

void MainWindow::on_animFlipX_cb_stateChanged(int arg1)
{
    int index = ui->animFrame_cb->currentIndex();
    this->bannerBin.animData[index].flipH = arg1 == Qt::Checked;
}

void MainWindow::on_animFlipY_cb_stateChanged(int arg1)
{
    int index = ui->animFrame_cb->currentIndex();
    this->bannerBin.animData[index].flipV = arg1 == Qt::Checked;
}

void MainWindow::on_actionAnimation_Player_triggered()
{
    int frameCount = ui->animFrame_cb->count();
    if(frameCount != 0)
    {
        NDSBannerAnimPlayer animPlayer(this, &this->bannerBin);
        animPlayer.exec();
    }
    else
    {
        QMessageBox::information(this, tr("Oops!"), tr("This banner has no frames yet."));
    }
}

void MainWindow::on_gfxImport_pb_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "", this->lastDirPath, tr("PNG Files") + " (*.png)");
    if(fileName == "")
        return;
    QFileInfo fileInfo(fileName);
    this->lastDirPath = fileInfo.dir().path();

    int bmpID = ui->gfxBmp_sb->value() - 1;
    int palID = ui->gfxPal_sb->value() - 1;

    u8* ncg;
    u16* ncl;
    getBinaryIconPtr(ncg, ncl, bmpID, palID);

    bool newPalette = true;
    if(bmpID != -1)
    {
        QMessageBox::StandardButton btn = QMessageBox::question(this, tr("Palette Replacement"), tr("Do you with to recreate the selected palette?"));
        if(btn == QMessageBox::No)
            newPalette = false;
    }

    QImage img(fileName);
    if(img.width() != 32 || img.height() != 32)
    {
        QMessageBox::critical(this, tr("faTal mEga eRrOR"), tr("Unfortunately??\nyes, unfortunately, the imported image is not 32x32 pixels."));
        return;
    }

    QNDSImage ndsImg;

    if (newPalette)
    {
        ndsImg.replace(img, 16, 0x80);
    }
    else
    {
        QVector<u16> pltt = QVector<u16>(ncl, ncl + 0x10);
        ndsImg.replace(img, pltt, 0x80);
    }

    QVector<u8> ncgV;
    QVector<u16> nclV;
    ndsImg.toNitro(ncgV, nclV, true);
    memcpy(ncg, ncgV.data(), 0x200);
    memcpy(ncl, nclV.data(), 0x20);

    updateIconView(bmpID, palID);
}

void MainWindow::on_gfxExport_pb_clicked()
{
    int bmpID = ui->gfxBmp_sb->value() - 1;
    int palID = ui->gfxPal_sb->value() - 1;
    QImage image = getCurrentImage(bmpID, palID);

    QString fileName = QFileDialog::getSaveFileName(this, "", this->lastDirPath, tr("PNG Files") + " (*.png)");
    if(fileName == "")
        return;
    QFileInfo fileInfo(fileName);
    this->lastDirPath = fileInfo.dir().path();

    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(this, "Big OOF", tr("Could not open file for writing."));
        return;
    }
    image.save(&file, "PNG");
    file.close();
}
