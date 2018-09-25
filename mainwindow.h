#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionOpen_triggered();

    void on_actionExit_2_triggered();

    bool eventFilter(QObject *obj, QEvent *event);
    void closeEvent(QCloseEvent *event);
    void resizeEvent(QResizeEvent* event);

    void redrawSlice();

    void updateStatusBar();

    void on_sldSlices_sliderMoved(int position);

    void on_sldSlices_valueChanged(int value);

    void on_cbMarkersVisible_clicked();

    void on_sldToolSize_sliderMoved(int position);

    void on_sldToolSize_valueChanged(int value);

    void on_btnDispDefault_clicked();

    void on_actionSave_triggered();

    void on_actionAbout_triggered();

    void on_actionInfo_triggered();

    void on_sldOpacity_sliderMoved(int position);

    void on_sldOpacity_valueChanged(int value);

    void on_sldLevelHU_sliderMoved(int position);

    void on_sldLevelHU_valueChanged(int value);

    void on_actionLung_700_1500_triggered();

    void on_actionChest_40_400_triggered();

    void on_actionBone_500_2000_triggered();

    void on_sldWidthHU_sliderMoved(int position);

    void on_sldWidthHU_valueChanged(int value);

    void on_actionSmall_triggered();

    void on_actionMedium_triggered();

    void on_actionLarge_triggered();

    void on_actionMaximum_triggered();

    void on_btnUpdateProj_clicked();

    void on_sldThres_sliderMoved(int position);

    void on_sldThres_valueChanged(int value);

    void on_actionExport_triggered();

    void on_cbOrigSliceLabels_clicked();

private:
    Ui::MainWindow *ui;
    void processWheelEvent(QObject *obj, QEvent *event);
    void processKeyEvent(QObject *obj, QEvent *event);
    void processMouseMove(QObject *obj, QEvent *event);
    short getCurrentTool();
    void colorizeRadioButton(int r, int g, int b, QWidget * widget);
    void setLevelAndWidthHU(int level, int width);
    void assignPalette();
    void filLabelImages();
    void prepareFrontalAndSagital();
    void updateFrontalAndSagitalLabels();
    void putLabel(QPoint pos, bool leftButton);
    bool checkUnsavedChanges();
    void updateMainSize();
    void drawMainImage(QPoint pos, double alpha, double beta);
    void drawFrontalImage(QPoint pos, double alpha, double beta);
    void drawSagitalImage(QPoint pos, double alpha, double beta);
    void recalcSliceLabels();
    void processClickLabel(QMouseEvent* me, QPoint pos);
    void processClickRuler(QPoint pos, QEvent *event);
    void processClickErode(QPoint pos, QEvent *event);
    void pickLabel(QPoint pos);
};

#endif // MAINWINDOW_H
