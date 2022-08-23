#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "types.h"

#include <QMainWindow>
#include <QGraphicsScene>

enum class ProgramState
{
    Closed,
    NewFile,
    KnowsPath
};

struct NDSBanner
{
    u16 version;
    u16 crc[4];
    u8 reserved[0x16];

    u8 iconNCG[0x200];
    u16 iconNCL[0x10];

    QChar title[16][0x100 / sizeof(QChar)];

    u8 iconExtraNCG[8][0x200];
    u16 iconExtraNCL[8][0x10];

    struct AnimSeq
    {
        u16 frameDuration : 8;
        u16 ncgID : 3;
        u16 nclID : 3;
        u16 flipH : 1;
        u16 flipV : 1;
    };

    AnimSeq animData[0x80 / sizeof(AnimSeq)];
};

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionOpen_triggered();

    void on_gameTitle_pte_textChanged();

    void on_gameTitle_cb_currentIndexChanged(int index);

    void on_gameTitle_pb_clicked();

    void on_bannerVersion_cb_currentIndexChanged(int index);

    void on_actionSave_triggered();

    void on_actionSave_As_triggered();

    void on_actionCredits_triggered();

    void on_actionQt_triggered();

    void on_gfxBmp_sb_valueChanged(int arg1);

    void on_gfxPal_sb_valueChanged(int arg1);

    void on_actionClose_triggered();

    void on_animFrame_cb_currentIndexChanged(int index);

    void on_animFrameAdd_pb_clicked();

    void on_animFrameRem_pb_clicked();

    void on_animDur_sb_valueChanged(int arg1);

    void on_animBmp_sb_valueChanged(int arg1);

    void on_animPal_sb_valueChanged(int arg1);

    void on_animFlipX_cb_stateChanged(int arg1);

    void on_animFlipY_cb_stateChanged(int arg1);

    void on_actionNew_triggered();

    void on_actionAnimation_Player_triggered();

    void on_gfxImport_pb_clicked();

    void on_gfxExport_pb_clicked();

private:
    Ui::MainWindow *ui;

    NDSBanner bannerBin;
    QString lastDirPath;
    QString openedFileName;
    QGraphicsScene gfx_scene;

    QImage getCurrentImage(int bmpID, int palID);
    QPixmap getCurrentPixmap(int bmpID, int palID);

    int gfxBmp_lastValue = 0;
    int animFrame_lastSize = 0;

    void setProgramState(ProgramState mode);

    void getBinaryIconPtr(u8*& ncg, u16*& ncl, int bmpID, int palID);
    void updateIconView(int bmpID, int ncgID);

    bool checkIfAllowClose();
    void closeEvent(QCloseEvent *event);

    void saveFile(const QString& path);

    void setAnimGroupBlockSignals(bool flag);
    void setAnimGroupEnabled(bool flag);
};
#endif // MAINWINDOW_H
