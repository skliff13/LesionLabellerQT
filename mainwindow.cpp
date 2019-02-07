#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "src/stdafx.h"
#include <QFileDialog>
#include <QDesktopServices>

#define WNDTITLE "Lesion Labeller v1.8.6"
#define PDFFILENAME "LesionLabellerQT_Doc.pdf"
#define MAXRECUR 333494

AnalyzeImage im3;
AnalyzeImage lbl;
Vector<Mat> frontalSlices;
Vector<Mat> frontalLabels;
Vector<Mat> sagitalSlices;
Vector<Mat> sagitalLabels;
Mat sliceLabels;
Mat origSliceLabels;
int wheelCounter = 0;
int keyCounter = 0;
QPoint midPos(-1, -1);
QPointF lastPos(-100, -100);

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    qApp->installEventFilter(this);
    ui->setupUi(this);

    this->setWindowTitle(WNDTITLE);

    UserData * ud = getUserData();
    setLevelAndWidthHU(-700, 1500);
    ud->toolSize = 4;
    ud->mainSize = 512;
    ud->rulerDist = -2;
    ud->rulerPoints[0] = Point3i(-1, -1, -1);
    ud->rulerPoints[1] = Point3i(-1, -1, -1);
    assignPalette();
    filLabelImages();

    ui->sldToolSize->setSliderPosition(ud->toolSize);

    ui->cbMarkersVisible->setChecked(true);
    ui->sldOpacity->setEnabled(ui->cbMarkersVisible->isChecked());
    ui->gbWait->hide();

    ud->HUshift = -1024;

    QString logfile = QCoreApplication::applicationDirPath().append("/log.txt");
    LOG_Begin((char *)logfile.toLatin1().data());
    LOG(WNDTITLE);
}

void MainWindow::filLabelImages()
{
    UserData * ud = getUserData();

    Mat empty512(IMMAINH, IMMAINW, CV_8UC3, Scalar::all(128));
    empty512(Range(0, IMMAINH), Range(0, 1)) = Scalar::all(0);
    empty512(Range(0, IMMAINH), Range(IMMAINW - 1, IMMAINW)) = Scalar::all(0);
    empty512(Range(0, 1), Range(0, IMMAINW)) = Scalar::all(0);
    empty512(Range(IMMAINH - 1, IMMAINH), Range(0, IMMAINW)) = Scalar::all(0);
    Mat res;
    cv::resize(empty512, res, Size(ud->mainSize, ud->mainSize), 0, 0, INTER_NEAREST);
    ui->lblMainImage->setPixmap(QPixmap::fromImage(QImage(res.data, res.cols, res.rows, res.step, QImage::Format_RGB888)));
    cv::resize(empty512, res, Size(ui->lblSliceLabels->width(), ud->mainSize), 0, 0, INTER_NEAREST);
    ui->lblSliceLabels->setPixmap(QPixmap::fromImage(QImage(res.data, res.cols, res.rows, res.step, QImage::Format_RGB888)));

    Mat empty256(IMPROJH, IMPROJW, CV_8UC3, Scalar::all(128));
    empty256(Range(0, IMPROJH), Range(0, 1)) = Scalar::all(0);
    empty256(Range(0, IMPROJH), Range(IMPROJW - 1, IMPROJW)) = Scalar::all(0);
    empty256(Range(0, 1), Range(0, IMPROJW)) = Scalar::all(0);
    empty256(Range(IMPROJH - 1, IMPROJH), Range(0, IMPROJW)) = Scalar::all(0);
    ui->lblProjFront->setPixmap(QPixmap::fromImage(QImage(empty256.data, empty256.cols, empty256.rows, empty256.step, QImage::Format_RGB888)));
    ui->lblProjSag->setPixmap(QPixmap::fromImage(QImage(empty256.data, empty256.cols, empty256.rows, empty256.step, QImage::Format_RGB888)));
}

void MainWindow::assignPalette()
{
    UserData * ud = getUserData();

    ud->palette.push_back(Vec3b(150, 250, 250));
    ud->palette.push_back(Vec3b(100, 0, 0));        // Focus < 10 mm
    ud->palette.push_back(Vec3b(170, 0, 0));        // Focus 10 - 30 mm
    ud->palette.push_back(Vec3b(250, 0, 0));        // Infiltrate
    ud->palette.push_back(Vec3b(200, 0, 200));      // Mix1
    ud->palette.push_back(Vec3b(200, 100, 200));    // Mix2
    ud->palette.push_back(Vec3b(0, 0, 100));        // Caverns
    ud->palette.push_back(Vec3b(0, 150, 150));      // Fibrosis
    ud->palette.push_back(Vec3b(50, 150, 50));      // Plevritis
    ud->palette.push_back(Vec3b(200, 150, 50));     // Atelectasis
    ud->palette.push_back(Vec3b(150, 150, 0));      // Pneumathorax

    Vec3b clr;
    clr = ud->palette[1];
    colorizeRadioButton(clr[0], clr[1], clr[2], ui->rbFocus10);
    clr = ud->palette[2];
    colorizeRadioButton(clr[0], clr[1], clr[2], ui->rbFocus30);
    clr = ud->palette[3];
    colorizeRadioButton(clr[0], clr[1], clr[2], ui->rbInfiltrate);
    clr = ud->palette[4];
    colorizeRadioButton(clr[0], clr[1], clr[2], ui->rbMix1);
    clr = ud->palette[5];
    colorizeRadioButton(clr[0], clr[1], clr[2], ui->rbMix2);
    clr = ud->palette[6];
    colorizeRadioButton(clr[0], clr[1], clr[2], ui->rbCaverns);
    clr = ud->palette[7];
    colorizeRadioButton(clr[0], clr[1], clr[2], ui->rbFibrosis);
    clr = ud->palette[8];
    colorizeRadioButton(clr[0], clr[1], clr[2], ui->rbPlevritis);
    clr = ud->palette[9];
    colorizeRadioButton(clr[0], clr[1], clr[2], ui->rbAtelectasis);
    clr = ud->palette[10];
    colorizeRadioButton(clr[0], clr[1], clr[2], ui->rbPneumathorax);
}

void MainWindow::prepareFrontalAndSagital(){
    UserData * ud = getUserData();

    int k;
    for (k = 0; k < frontalSlices.size(); k++)
        frontalSlices[k].release();
    for (k = 0; k < sagitalSlices.size(); k++)
        sagitalSlices[k].release();

    frontalSlices.clear();
    sagitalSlices.clear();

    int nz0 = im3.slices.size();
    k = 0;

    for (int j = 0; j < im3.slices[0].cols; j++){
        Mat im0(nz0, im3.slices[0].rows, CV_16SC1, Scalar::all(-5000));

        for (k = 0; k < nz0; k++){
            Mat slcol = im3.slices[k].col(j);
            Mat imrow = im0.row(nz0 - k - 1);

            for (int l = 0; l < slcol.rows; l++){
                imrow.at<short>(0, l) = slcol.at<short>(l, 0);
            }
        }

        Mat im1;
        cv::resize(im0, im1, Size(im0.cols, round(im0.rows * ud->z2xy)));
        sagitalSlices.push_back(im1);
    }

    for (int i = 0; i < im3.slices[0].rows; i++){
        Mat im0(nz0, im3.slices[0].cols, CV_16SC1);

        for (k = 0; k < nz0; k++){
            Mat slrow = im3.slices[k].row(i);
            Mat imrow = im0.row(nz0 - k - 1);
            slrow.copyTo(imrow);
        }

        Mat im1;
        cv::resize(im0, im1, Size(im0.cols, round(im0.rows * ud->z2xy)));
        frontalSlices.push_back(im1);
    }

    updateFrontalAndSagitalLabels();
}

void MainWindow::updateFrontalAndSagitalLabels(){
    UserData * ud = getUserData();

    int k;
    for (k = 0; k < frontalLabels.size(); k++)
        frontalLabels[k].release();
    for (k = 0; k < sagitalLabels.size(); k++)
        sagitalLabels[k].release();

    frontalLabels.clear();
    sagitalLabels.clear();

    int nz0 = im3.slices.size();
    k = 0;

    for (int j = 0; j < im3.slices[0].cols; j++){
        Mat lb0(nz0, im3.slices[0].rows, CV_16SC1, Scalar::all(0));

        for (k = 0; k < nz0; k++){
            Mat lbcol = lbl.slices[k].col(j);
            Mat l0row = lb0.row(nz0 - k - 1);

            for (int l = 0; l < lbcol.rows; l++){
                l0row.at<short>(0, l) = lbcol.at<char>(l, 0);
            }
        }

        Mat lb1;
        cv::resize(lb0, lb1, Size(lb0.cols, round(lb0.rows * ud->z2xy)), 0, 0, CV_INTER_NN);
        lb1.convertTo(lb1, CV_8SC1);
        sagitalLabels.push_back(lb1);
    }

    for (int i = 0; i < im3.slices[0].rows; i++){
        Mat lb0(nz0, im3.slices[0].cols, CV_8SC1);

        for (k = 0; k < nz0; k++){
            Mat lbrow = lbl.slices[k].row(i);
            Mat l0row = lb0.row(nz0 - k - 1);
            lbrow.copyTo(l0row);
        }

        Mat lb1;
        lb0.convertTo(lb0, CV_16SC1);
        cv::resize(lb0, lb1, Size(lb0.cols, round(lb0.rows * ud->z2xy)), 0, 0, CV_INTER_NN);
        lb1.convertTo(lb1, CV_8SC1);
        frontalLabels.push_back(lb1);
    }
}

void MainWindow::setLevelAndWidthHU(int level, int width)
{       
    ui->sldLevelHU->setSliderPosition(level);
    ui->sldWidthHU->setSliderPosition(width);

    ui->lblLevelHU->setText(QString("Level: %1").arg(ui->sldLevelHU->sliderPosition()));
    ui->lblWidthHU->setText(QString("Width: %1").arg(ui->sldWidthHU->sliderPosition()));

    redrawSlice();
}

void MainWindow::colorizeRadioButton(int r, int g, int b, QWidget * widget)
{
    QColor qclr(r, g, b);
    QPalette pal = widget->palette();
    pal.setColor(QPalette::WindowText, qclr);
    widget->setPalette(pal);
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::checkUnsavedChanges()
{
    UserData * ud = getUserData();

    if (!ud->unsavedChanges){
        return true;
    }
    else {
        QMessageBox::StandardButton resBtn = QMessageBox::question(
                    this, "Lesion labeller", tr("Save changes before quit?\n"),
                    QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);

        if (resBtn == QMessageBox::Yes) {
            on_actionSave_triggered();
            return true;
        }
        if (resBtn == QMessageBox::No) {
            return true;
        }
        if (resBtn == QMessageBox::Cancel) {
            return false;
        }
    }

    return true;
}

void NormalizeAnalyzeImage(AnalyzeImage &im){
    int mid = im.slices.size() / 2;
    Mat midSlice = im.slices[mid];
    Mat center;
    midSlice(Range(128, 384), Range(128, 384)).copyTo(center);

    double minVal = 0.;
    minMaxLoc(center, &minVal);

    if (minVal < -500){
        LOG(QString("Normalizing image: +1024 HU").toLatin1().data());

        for (int k = 0; k < im.slices.size(); k++){
            im.slices[k] = im.slices[k] + 1024.;
        }
    }
}

void MainWindow::on_actionOpen_triggered()
{
    UserData * ud = getUserData();

    if (!checkUnsavedChanges())
        return;

    QString dir = QCoreApplication::applicationDirPath();
    QString filepath = QFileDialog::getOpenFileName(this, tr("Open File"), dir, tr("Analyze Images (*.hdr *.img)"));

    if (!filepath.isEmpty()){
        QString origFilepath;
        QString lblFilepath;

        ui->lblWaitMsg->setText("Loading image ...");
        ui->gbWait->show();
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        filepath.replace(".img", ".hdr");
        if (filepath.contains("_lesions.")){
            lblFilepath = filepath;
            int l = filepath.lastIndexOf("_lesions.");
            origFilepath = filepath.left(l).append(".hdr");
        }
        else {
            origFilepath = filepath;
            lblFilepath = filepath.replace(".hdr", "_lesions.hdr");
        }
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        Analyze75Read(origFilepath.toLatin1().data(), &im3);
        NormalizeAnalyzeImage(im3);

        strcpy(ud->filePath, origFilepath.toLatin1().data());

        QFileInfo fileInfo(lblFilepath);
        if (!fileInfo.exists()){
            LOG(QString("No labelling found for '").append(origFilepath).append("'. Starting from scratch.").toLatin1().data());

            lbl.hdrInfo = im3.hdrInfo;
            lbl.slices.clear();
            for (int k = 0; k < im3.slices.size(); k++){
                Mat m0 = im3.slices[k], m;
                m0.convertTo(m, CV_8SC1, 0.);
                lbl.slices.push_back(m);
            }
        }
        else {
            LOG(QString("Labelling found for '").append(origFilepath).append("'.").toLatin1().data());

            Analyze75Read(lblFilepath.toLatin1().data(), &lbl, true);
        }

        ud->curSlice = im3.slices.size() / 2;
        ui->sldSlices->setMaximum(im3.slices.size() - 1);
        ud->z2xy = im3.hdrInfo.dims.pixdim[3] / im3.hdrInfo.dims.pixdim[1];

        QFileInfo fi(ud->filePath);
        this->setWindowTitle(QString(WNDTITLE).append(" - ").append(fi.fileName()));
        ud->unsavedChanges = false;

        prepareFrontalAndSagital();
        recalcSliceLabels();
        sliceLabels.copyTo(origSliceLabels);

        ui->gbWait->hide();

        redrawSlice();
    }
}

void MainWindow::on_actionSave_triggered()
{
    UserData * ud = getUserData();

    if (!im3.slices.empty()){
        QString origFilepath(ud->filePath);
        LOG(QString("Saving labelling for '").append(origFilepath).append("'.").toLatin1().data());

        QString lblFilepath = origFilepath.replace(".hdr", "_lesions.hdr");
        Analyze75Write(lblFilepath.toLatin1().data(), &lbl, true);

        QFileInfo fi(ud->filePath);
        this->setWindowTitle(QString(WNDTITLE).append(" - ").append(fi.fileName()));
        ud->unsavedChanges = false;
        statusBar()->showMessage("Changes saved");
    }
}

void MainWindow::on_actionExit_2_triggered()
{  
    this->close();
}

void MainWindow::processWheelEvent(QObject *obj, QEvent *event)
{
    UserData * ud = getUserData();

    QWheelEvent * we = (QWheelEvent *)event;

    if (we->angleDelta().y() > 0){
        if (obj == this){
            ud->curSlice++;
            ud->curSlice = ud->curSlice < int(im3.slices.size()) ? ud->curSlice : im3.slices.size() - 1;
            redrawSlice();
        }
    }
    if (we->angleDelta().y() < 0){
        if (obj == this){
            ud->curSlice--;
            ud->curSlice = ud->curSlice >= 0 ? ud->curSlice : 0;
            redrawSlice();
        }
    }
}

void MainWindow::processKeyEvent(QObject *obj, QEvent *event)
{
    if (obj == this){
        UserData * ud = getUserData();
        QKeyEvent * ke = (QKeyEvent *)event;

        if (ke->text() == "o" || ke->text() == "O" || ke->text() == "щ" || ke->text() == "Щ")
            ui->cbOrigSliceLabels->setChecked(!ui->cbOrigSliceLabels->isChecked());
        if (ke->text() == "v" || ke->text() == "V" || ke->text() == "м" || ke->text() == "М")
            ui->cbMarkersVisible->setChecked(!ui->cbMarkersVisible->isChecked());
        if (ke->text() == "t" || ke->text() == "T" || ke->text() == "е" || ke->text() == "Е")
            ui->cbThresholded->setChecked(!ui->cbThresholded->isChecked());
        if (ke->text() == "d" || ke->text() == "D" || ke->text() == "в" || ke->text() == "В")
            ui->cbTool3D->setChecked(!ui->cbTool3D->isChecked());
        if (ke->text() == "`" || ke->text() == "~" || ke->text() == "ё" || ke->text() == "Ё")
            ui->rbRubber->setChecked(true);
        if (ke->text() == "r" || ke->text() == "R" || ke->text() == "к" || ke->text() == "К")
            on_btnUpdateProj_clicked();
        if (ke->text() == "l" || ke->text() == "L" || ke->text() == "д" || ke->text() == "Д"){
            ud->rulerDist = -2;
            ui->rbRuler->setChecked(true);
        }
        if (ke->text() == "e" || ke->text() == "E" || ke->text() == "у" || ke->text() == "У")
            ui->rbErode->setChecked(true);
        if (ke->text() == "1")
            ui->rbFocus10->setChecked(true);
        if (ke->text() == "2")
            ui->rbFocus30->setChecked(true);
        if (ke->text() == "3")
            ui->rbInfiltrate->setChecked(true);
        if (ke->text() == "4")
            ui->rbMix1->setChecked(true);
        if (ke->text() == "5")
            ui->rbMix2->setChecked(true);
        if (ke->text() == "6")
            ui->rbCaverns->setChecked(true);
        if (ke->text() == "7")
            ui->rbFibrosis->setChecked(true);
        if (ke->text() == "8")
            ui->rbPlevritis->setChecked(true);
        if (ke->text() == "9")
            ui->rbAtelectasis->setChecked(true);
        if (ke->text() == "0")
            ui->rbPneumathorax->setChecked(true);
        if (ke->text() == "+" || ke->text() == "="){
            if (ui->sldToolSize->sliderPosition() < ui->sldToolSize->maximum())
                ud->toolSize++;
        }
        if (ke->text() == "-"){
            if (ui->sldToolSize->sliderPosition() > ui->sldToolSize->minimum())
                ud->toolSize--;
        }
        if (ke->text() == "]" || ke->text() == "ъ" || ke->text() == "Ъ"){
            if (ui->sldThres->sliderPosition() <= ui->sldThres->maximum() - ui->sldThres->pageStep())
                ui->sldThres->setSliderPosition(ui->sldThres->sliderPosition() + ui->sldThres->pageStep());
        }
        if (ke->text() == "[" || ke->text() == "х" || ke->text() == "Х"){
            if (ui->sldThres->sliderPosition() >= ui->sldThres->minimum() + ui->sldThres->pageStep())
                ui->sldThres->setSliderPosition(ui->sldThres->sliderPosition() - ui->sldThres->pageStep());
        }

        redrawSlice();
    }
}

short MainWindow::getCurrentTool()
{
    short label = 1 * ui->rbFocus10->isChecked() + 2 * ui->rbFocus30->isChecked() + 3 * ui->rbInfiltrate->isChecked()
            + 4 * ui->rbMix1->isChecked() + 5 * ui->rbMix2->isChecked() + 6 * ui->rbCaverns->isChecked()
            + 7 * ui->rbFibrosis->isChecked() + 8 * ui->rbPlevritis->isChecked()
            + 9 * ui->rbAtelectasis->isChecked() + 10 * ui->rbPneumathorax->isChecked();

    return label;
}

void customCircle(Mat lblIm, Mat origIm, Point c, int r, Scalar clr, double thres, bool leftButton){
    if (thres <= -2000){
        circle(lblIm, c, r, clr, -1);
    }
    else {
        for (int i = c.y - r; i <= c.y + r; i++){
            for (int j = c.x - r; j <= c.x + r; j++){
                int di = i - c.y;
                int dj = j - c.x;
                if (i >= 0 && i < lblIm.rows && j >= 0 && j < lblIm.cols && sqr(di) + sqr(dj) <= sqr(r)){
                    if (leftButton && origIm.at<short>(i, j) + getUserData()->HUshift >= thres){
                        lblIm.at<char>(i, j) = (char)clr[0];
                    }
                    if (!leftButton && origIm.at<short>(i, j) + getUserData()->HUshift < thres){
                        lblIm.at<char>(i, j) = (char)clr[0];
                    }
                }
            }
        }
    }
}

void MainWindow::putLabel(QPoint pos, bool leftButton)
{
    UserData * ud = getUserData();

    int r = ud->toolSize * TOOLMULT;
    short label = getCurrentTool();

    double thres = ui->cbThresholded->isChecked() ? ui->sldThres->value() : -3000;

    if (!ui->cbTool3D->isChecked()){
        Mat lblSlice = lbl.slices[ud->curSlice];
        Mat imSlice = im3.slices[ud->curSlice];
        customCircle(lblSlice, imSlice, Point(pos.x(), lblSlice.rows - 1 - pos.y()), r, Scalar::all(label), thres, leftButton);
    }
    else {
        int maxdk = (int)floor(r / ud->z2xy);

        for (int dk = -maxdk; dk <= maxdk; dk++){
            if (ud->curSlice + dk >= 0 && ud->curSlice + dk < im3.slices.size() && sqr(dk * ud->z2xy) < sqr(r)){
                Mat lblSlice = lbl.slices[ud->curSlice + dk];
                Mat imSlice = im3.slices[ud->curSlice + dk];
                int r1 = sqrt(sqr(r) - sqr(dk * ud->z2xy));
                customCircle(lblSlice, imSlice, Point(pos.x(), lblSlice.rows - 1 - pos.y()), r1, Scalar::all(label), thres, leftButton);
            }
        }

        int projY = frontalLabels[0].rows - 1 - round(ud->curSlice * ud->z2xy);

        for (int f = -r; f <= r; f++){
            int fslice = lbl.slices[0].rows - 1 - pos.y() + f;
            if (fslice >= 0 && fslice < frontalLabels.size()){
                int r1 = sqrt(sqr(r) - sqr(f));
                customCircle(frontalLabels[fslice], frontalSlices[fslice], Point(pos.x(), projY), r1, Scalar::all(label), thres, leftButton);
            }

            int sslice = pos.x() + f;
            if (sslice >= 0 && sslice < sagitalLabels.size()){
                int r1 = sqrt(sqr(r) - sqr(f));
                customCircle(sagitalLabels[sslice], sagitalSlices[sslice], Point(sagitalLabels[0].cols - 1 - pos.y(), projY), r1, Scalar::all(label), thres, leftButton);
            }
        }
    }
}

void MainWindow::pickLabel(QPoint pos)
{
    UserData * ud = getUserData();

    Mat lblSlice = lbl.slices[ud->curSlice];
    int l = lblSlice.at<char>(lblSlice.rows - 1 - pos.y(), pos.x());
    switch (l) {
    case 0:
        ui->rbRubber->setChecked(true);
        break;
    case 1:
        ui->rbFocus10->setChecked(true);
        break;
    case 2:
        ui->rbFocus30->setChecked(true);
        break;
    case 3:
        ui->rbInfiltrate->setChecked(true);
        break;
    case 4:
        ui->rbMix1->setChecked(true);
        break;
    case 5:
        ui->rbMix2->setChecked(true);
        break;
    case 6:
        ui->rbCaverns->setChecked(true);
        break;
    case 7:
        ui->rbFibrosis->setChecked(true);
        break;
    case 8:
        ui->rbPlevritis->setChecked(true);
        break;
    case 9:
        ui->rbAtelectasis->setChecked(true);
        break;
    case 10:
        ui->rbPneumathorax->setChecked(true);
        break;
    default:
        break;
    }
}

void MainWindow::processClickLabel(QMouseEvent* me, QPoint pos)
{
    UserData * ud = getUserData();

    if (!ui->cbThresholded->isChecked() && me->buttons() == Qt::RightButton){
        pickLabel(pos);
    }
    else{
        putLabel(pos, me->buttons() == Qt::LeftButton);

        if (!ud->unsavedChanges){
            ud->unsavedChanges = true;
            QFileInfo fi(ud->filePath);
            this->setWindowTitle(QString(WNDTITLE).append(" - ").append(fi.fileName()).append(" *"));
        }
    }
}

void MainWindow::processClickRuler(QPoint pos, QEvent *event)
{
    UserData * ud = getUserData();

    if (event->type() == QEvent::MouseButtonPress){
        if (ud->rulerDist < -0.5 && ud->rulerDist > -1.5){
            ud->rulerPoints[1] = Point3i(pos.x(), pos.y(), ud->curSlice);
            Point3i dp = ud->rulerPoints[1] - ud->rulerPoints[0];
            float * pxdim = im3.hdrInfo.dims.pixdim;
            ud->rulerDist = sqrt(sqr(dp.x * pxdim[1]) + sqr(dp.y * pxdim[2]) + sqr(dp.z * pxdim[3]));
        }
        else{
            ud->rulerDist = -1;
            ud->rulerPoints[0] = Point3i(pos.x(), pos.y(), ud->curSlice);
        }
    }
}

bool reccurIsBeyond(AnalyzeImage & visited, char label, Point3i p, int counter, bool & success){
    if (!success)
        return false;
    if (counter > MAXRECUR - 10){
        success = false;
        return false;
    }

    if (p.x < 0 || p.x > lbl.slices[0].cols || p.y < 0 || p.y > lbl.slices[0].rows)
        return false;
    if (p.z < 0 || p.z > lbl.slices.size())
        return false;
    if (visited.slices[p.z].at<char>(p.y, p.x) == 1)
        return false;

    visited.slices[p.z].at<char>(p.y, p.x) = 1;

    int dx, dy;
    bool isBeyond = true;
    for (dx = -1; dx <= 1; dx++){
        for (dy = -1; dy <= 1; dy++){
            char l = lbl.slices[p.z].at<char>(p.y + dy, p.x + dx);
            isBeyond = isBeyond && (l != label);
        }
    }

    if (isBeyond)
        return true;

    bool isBorder = false;
    isBorder = isBorder || reccurIsBeyond(visited, label, p + Point3i(3, 0, 0), counter + 1, success);
    isBorder = isBorder || reccurIsBeyond(visited, label, p + Point3i(-3, 0, 0), counter + 1, success);
    isBorder = isBorder || reccurIsBeyond(visited, label, p + Point3i(0, 3, 0), counter + 1, success);
    isBorder = isBorder || reccurIsBeyond(visited, label, p + Point3i(0, -3, 0), counter + 1, success);
    isBorder = isBorder || reccurIsBeyond(visited, label, p + Point3i(0, 0, 1), counter + 1, success);
    isBorder = isBorder || reccurIsBeyond(visited, label, p + Point3i(0, 0, -1), counter + 1, success);

    if (isBorder){
        for (dx = -1; dx <= 1; dx++){
            for (dy = -1; dy <= 1; dy++){
                lbl.slices[p.z].at<char>(p.y + dy, p.x + dx) = 0;
            }
        }
    }

    return false;
}

bool neighbourIsAddable(Point3i neigh, char label){
    if (neigh.x < 1 || neigh.x > lbl.slices[0].cols - 1 ||
            neigh.y < 1 || neigh.y > lbl.slices[0].rows - 1 ||
            neigh.z < 1 || neigh.z > lbl.slices.size() - 1)
        return false;

    bool allBeyondLabel = true;
    for (int dx = -1; dx <= 1; dx++){
        for (int dy = -1; dy <= 1; dy++){
            char l = lbl.slices[neigh.z].at<char>(neigh.y + dy, neigh.x + dx);
            allBeyondLabel = allBeyondLabel && (l != label);
        }
    }
    if (allBeyondLabel)
        return false;

    return true;
}

bool queueErode(AnalyzeImage & visited, char label, Point3i p0){
    QQueue<Point3i> q;
    q.enqueue(p0);
    visited.slices[p0.z].at<char>(p0.y, p0.x) = 1;

    Vector<Point3i> deltas;
    deltas.push_back(Point3i(3, 0, 0));
    deltas.push_back(Point3i(-3, 0, 0));
    deltas.push_back(Point3i(0, 3, 0));
    deltas.push_back(Point3i(0, -3, 0));
    deltas.push_back(Point3i(0, 0, 1));
    deltas.push_back(Point3i(0, 0, -1));

    int dx, dy;
    while (!q.isEmpty()){
        if (q.size() > 1000000){
            return false;
        }
        Point3i p = q.dequeue();

        bool voxIsInternal = true;
        for (int i = 0; i < deltas.size(); i++){
            Point3i neigh = p + deltas[i];

            bool neighIsAddable = neighbourIsAddable(neigh, label);

            if (neighIsAddable && (visited.slices[neigh.z].at<char>(neigh.y, neigh.x) == 0)){
                q.enqueue(neigh);
                visited.slices[neigh.z].at<char>(neigh.y, neigh.x) = 1;
            }
            voxIsInternal = voxIsInternal && neighIsAddable;
        }

        if (!voxIsInternal){
            for (dx = -1; dx <= 1; dx++){
                for (dy = -1; dy <= 1; dy++){
                    lbl.slices[p.z].at<char>(p.y + dy, p.x + dx) = 0;
                }
            }
        }
    }

    return true;
}

void MainWindow::processClickErode(QPoint pos, QEvent *event)
{
    UserData * ud = getUserData();

    if (event->type() == QEvent::MouseButtonPress){
        Mat lblSlice = lbl.slices[ud->curSlice];
        Point3i p = Point3i(pos.x(), lblSlice.rows - pos.y(), ud->curSlice);
        char label = lblSlice.at<char>(p.y, p.x);

        AnalyzeImage visited;
        visited.hdrInfo = im3.hdrInfo;
        visited.slices.clear();
        for (int k = 0; k < lbl.slices.size(); k++){
            Mat m0 = lbl.slices[k], m;
            m0.convertTo(m, m0.type(), 0.);
            visited.slices.push_back(m);
        }

        if (label > 0){
            LOG(QString("Eroding at Point = (%2, %3, %4), Label = %1").
                arg(int(label)).arg(p.x).arg(p.y).arg(p.z).toLatin1().data());

            if (!ud->unsavedChanges){
                ud->unsavedChanges = true;
                QFileInfo fi(ud->filePath);
                this->setWindowTitle(QString(WNDTITLE).append(" - ").append(fi.fileName()).append(" *"));
            }

            bool success = queueErode(visited, label, p);

            if (!success){
                QString mess = "Erosion not finished: Stack Overflow";
                LOG(mess.toLatin1().data());
                QMessageBox::warning(this, "Warning", mess);
            }
            else
                LOG("Erosion finished");
        }
    }
}

void MainWindow::processMouseMove(QObject *obj, QEvent *event)
{
    UserData * ud = getUserData();
    QMouseEvent * me = (QMouseEvent *)event;

    if (obj == ui->lblMainImage){
        lastPos = me->localPos();

        if (me->buttons() == Qt::LeftButton || me->buttons() == Qt::RightButton){
            QPoint pos(round(lastPos.rx()), round(lastPos.ry()));
            pos.setX(pos.x() * 512.0 / ud->mainSize);
            pos.setY(pos.y() * 512.0 / ud->mainSize);

            if (ud->curSliceImage.data && obj == ui->lblMainImage){                
                if (!ui->rbRuler->isChecked() && !ui->rbErode->isChecked()){
                    processClickLabel(me, pos);
                }
                if (ui->rbRuler->isChecked()){
                    processClickRuler(pos, event);
                }
                if (ui->rbErode->isChecked()){
                    processClickErode(pos, event);
                }
            }
            else {
                statusBar()->showMessage("No image loaded yet");
            }
        }
        if (me->buttons() == Qt::MiddleButton && obj == ui->lblMainImage){
            if (midPos.x() > 0){
                int deltaX = me->pos().x() - midPos.x();
                int deltaY = me->pos().y() - midPos.y();

                ui->sldLevelHU->setValue(ui->sldLevelHU->value() - 1 * deltaX);
                ui->sldWidthHU->setValue(ui->sldWidthHU->value() + 1 * deltaY);
            }

            midPos = me->pos();
        }

        if (me->buttons() != Qt::MiddleButton){
            midPos = QPoint(-1, -1);
        }

        redrawSlice();
        updateStatusBar();
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseMove)
    {
        processMouseMove(obj, event);
    }
    if (event->type() == QEvent::MouseButtonPress)
    {
        processMouseMove(obj, event);
    }
    if (event->type() == QEvent::Wheel){
        processWheelEvent(obj, event);
    }
    if (event->type() == QEvent::KeyPress){
        processKeyEvent(obj, event);
    }

    return false;
}

void MainWindow::updateStatusBar(){
    UserData * ud = getUserData();

    QPoint pos(round(lastPos.rx()), round(lastPos.ry()));
    pos.setX(pos.x() * 512.0 / ud->mainSize);
    pos.setY(pos.y() * 512.0 / ud->mainSize);

    if (ud->curSliceImage.data){
        if (!ui->rbRuler->isChecked()){
            if (pos.x() >= 0 && pos.x() < IMMAINW && pos.y() >= 0 && pos.y() < IMMAINH){
                Mat origSlice = im3.slices[ud->curSlice];
                short val = origSlice.at<short>(origSlice.rows - 1 - pos.y(), pos.x());
                Mat lblSlice = lbl.slices[ud->curSlice];
                int l = lblSlice.at<char>(origSlice.rows - 1 - pos.y(), pos.x());
                int HU = val + ud->HUshift;
                float d = (ud->toolSize * TOOLMULT - 1) * 2 * im3.hdrInfo.dims.pixdim[1];
                statusBar()->showMessage(QString("Slice = %1/%2, X = %3, Y = %4, D = %7 mm, class = %6, HU = %5")
                                         .arg(ud->curSlice + 1).arg(im3.slices.size()).arg(pos.x() + 1).arg(pos.y() + 1).arg(HU).arg(l).arg(int(d)));
            }
            else {
                statusBar()->showMessage(QString("Slice = %1/%2").arg(ud->curSlice + 1).arg(im3.slices.size()));
            }
        }
        else {
            if (ud->rulerDist < -1.5)
                statusBar()->showMessage(QString("Slice = %1/%2, X = %3, Y = %4. Ruler: mark point #1")
                                     .arg(ud->curSlice + 1).arg(im3.slices.size()).arg(pos.x() + 1).arg(pos.y() + 1));
            else if (ud->rulerDist < -0.5)
                statusBar()->showMessage(QString("Slice = %1/%2, X = %3, Y = %4. Ruler: mark point #2")
                                     .arg(ud->curSlice + 1).arg(im3.slices.size()).arg(pos.x() + 1).arg(pos.y() + 1));
            else if (ud->rulerDist > -0.5)
                statusBar()->showMessage(QString("Slice = %1/%2, X = %3, Y = %4. Ruler: distance = %5 mm")
                                     .arg(ud->curSlice + 1).arg(im3.slices.size()).arg(pos.x() + 1).arg(pos.y() + 1).arg(ud->rulerDist, 0, 'f', 1));
        }
    }
}

void putColorLabels(Mat rgb, Mat lbl2d, Ui::MainWindow * ui, bool invert = false)
{
    UserData * ud = getUserData();

    if (ui->cbMarkersVisible->isChecked()){
        for (int i = 0; i < rgb.rows; i++){
            for (int j = 0; j < rgb.cols; j++){
                int i1 = invert ? lbl2d.rows - i - 1 : i;
                char l = lbl2d.at<char>(i1, j);
                if (l > 0){
                    Vec3b clr = ud->palette[l];
                    Vec3b pix = rgb.at<Vec3b>(i, j);
                    double q = double(ui->sldOpacity->sliderPosition()) / ui->sldOpacity->maximum();
                    Vec3b res = Vec3b(clr[0] * q + pix[0] * (1 - q), clr[1] * q + pix[1] * (1 - q), clr[2] * q + pix[2] * (1 - q));
                    rgb.at<Vec3b>(i, j) = res;
                }
            }
        }
    }
}

void drawImageBorder(Mat rgb, Scalar brd)
{
    rgb(Range(0, rgb.rows), Range(0, 1)) = brd;
    rgb(Range(0, rgb.rows), Range(rgb.cols - 1, rgb.cols)) = brd;
    rgb(Range(0, 1), Range(0, rgb.cols)) = brd;
    rgb(Range(rgb.rows - 1, rgb.rows), Range(0, rgb.cols)) = brd;
}

Mat composeMainImage(QPoint pos, double alpha, double beta, Ui::MainWindow * ui){
    UserData * ud = getUserData();

    Mat rgb;
    im3.slices[ud->curSlice].convertTo(ud->curSliceImage, CV_8UC1, alpha, beta);
    flip(ud->curSliceImage, ud->curSliceImage, 0);
    cvtColor(ud->curSliceImage, rgb, CV_GRAY2BGR);

    Mat lslice = lbl.slices[ud->curSlice];
    putColorLabels(rgb, lslice, ui, true);    

    int r = ud->toolSize * TOOLMULT;

    if (!ui->rbRuler->isChecked() && !ui->rbErode->isChecked()){
        Vec3b clr = Vec3b(150, 250, 250);
        circle(rgb, Point(pos.x(), pos.y()), r, Scalar(clr[0], clr[1], clr[2]));
        if (!ui->cbTool3D->isChecked())
            rectangle(rgb, Point(pos.x() - r - 2, pos.y() - r - 2), Point(pos.x() + r + 2, pos.y() + r + 2), Scalar(clr[0], clr[1], clr[2]));
        if (ui->cbThresholded->isChecked())
            circle(rgb, Point(pos.x(), pos.y()), r + 2, Scalar(clr[2], clr[1], clr[0]));
    }
    else if (ui->rbRuler->isChecked()){
        Scalar c = Scalar(255, 128, 0);
        Point3i * ps = ud->rulerPoints;
        for (int i = 0; i < 2; i++){
            if (ud->curSlice == ps[i].z && ud->rulerDist > -1.5 + i){
                circle(rgb, Point(ps[i].x, ps[i].y), 1, c);
                circle(rgb, Point(ps[i].x, ps[i].y), 4, c);
            }
        }

        if (ud->rulerDist > -0.5 && ud->curSlice == ps[0].z && ud->curSlice == ps[1].z)
            line(rgb, Point(ps[0].x, ps[0].y), Point(ps[1].x, ps[1].y), c);
    }

    if (!ui->cbMarkersVisible->isChecked())
        drawImageBorder(rgb, Scalar(255, 0, 0));

    return rgb;
}

void MainWindow::drawMainImage(QPoint pos, double alpha, double beta)
{
    UserData * ud = getUserData();

    Mat rgb = composeMainImage(pos, alpha, beta, ui);

    Mat res;
    cv::resize(rgb, res, Size(ud->mainSize, ud->mainSize), 0, 0, INTER_NEAREST);
    ui->lblMainImage->setPixmap(QPixmap::fromImage(QImage(res.data, res.cols, res.rows, res.step, QImage::Format_RGB888)));

    if (!ui->cbOrigSliceLabels->isChecked())
        cv::resize(sliceLabels, res, Size(ui->lblSliceLabels->width(), ud->mainSize));
    else
        cv::resize(origSliceLabels, res, Size(ui->lblSliceLabels->width(), ud->mainSize));
    ui->lblSliceLabels->setPixmap(QPixmap::fromImage(QImage(res.data, res.cols, res.rows, res.step, QImage::Format_RGB888)));
}

void MainWindow::drawFrontalImage(QPoint pos, double alpha, double beta){
    UserData * ud = getUserData();

    int topSlice = round((im3.slices.size() - ud->curSlice) * ud->z2xy) - IMPROJH / 2;
    topSlice = topSlice < 0 ? 0 : topSlice;
    topSlice = topSlice + IMPROJH >= frontalSlices[0].rows ? frontalSlices[0].rows - IMPROJH - 1 : topSlice;
    int leftFront = pos.x() - IMPROJW / 2;
    leftFront = leftFront < 0 ? 0 : leftFront;
    leftFront = leftFront + IMPROJW >= IMMAINW ? IMMAINW - IMPROJW - 1 : leftFront;

    Mat selFront, imFront, rgbf, lblFront;
    selFront = frontalSlices[frontalSlices.size() - pos.y() - 1](Range(topSlice, topSlice + IMPROJH), Range(leftFront, leftFront + IMPROJW));
    selFront.convertTo(imFront, CV_8UC1, alpha, beta);
    lblFront = frontalLabels[frontalSlices.size() - pos.y() - 1](Range(topSlice, topSlice + IMPROJH), Range(leftFront, leftFront + IMPROJW));

    cvtColor(imFront, rgbf, CV_GRAY2BGR);
    putColorLabels(rgbf, lblFront, ui);

    int relSlice = frontalSlices[0].rows - topSlice - round(ud->curSlice * ud->z2xy);
    if (relSlice > 1 && relSlice < rgbf.rows - 1)
        rgbf(Range(relSlice, relSlice + 1), Range(0, IMPROJW)) = Scalar::all(128);
    int relX = pos.x() - leftFront;
    if (relX > 1 && relX < rgbf.cols - 1)
        rgbf(Range(0, IMPROJH), Range(relX, relX + 1)) = Scalar::all(128);

    Scalar brd = ui->cbMarkersVisible->isChecked() ? Scalar::all(0) : Scalar(255, 0, 0);
    drawImageBorder(rgbf, brd);

    ui->lblProjFront->setPixmap(QPixmap::fromImage(QImage(rgbf.data, rgbf.cols, rgbf.rows, rgbf.step, QImage::Format_RGB888)));
}

void MainWindow::drawSagitalImage(QPoint pos, double alpha, double beta){
    UserData * ud = getUserData();

    int topSlice = round((im3.slices.size() - ud->curSlice) * ud->z2xy) - IMPROJH / 2;
    topSlice = topSlice < 0 ? 0 : topSlice;
    topSlice = topSlice + IMPROJH >= frontalSlices[0].rows ? frontalSlices[0].rows - IMPROJH - 1 : topSlice;
    int leftSag = IMMAINH - pos.y() - IMPROJH / 2;
    leftSag = leftSag < 0 ? 0 : leftSag;
    leftSag = leftSag + IMPROJH >= IMMAINH ? IMMAINH - IMPROJH - 1 : leftSag;

    Mat selSag, imSag, rgbs, lblSag;
    selSag = sagitalSlices[pos.x()](Range(topSlice, topSlice + IMPROJH), Range(leftSag, leftSag + IMPROJW));
    selSag.convertTo(imSag, CV_8UC1, alpha, beta);
    lblSag = sagitalLabels[pos.x()](Range(topSlice, topSlice + IMPROJH), Range(leftSag, leftSag + IMPROJW));

    cvtColor(imSag, rgbs, CV_GRAY2BGR);
    putColorLabels(rgbs, lblSag, ui);

    int relSlice = frontalSlices[0].rows - topSlice - round(ud->curSlice * ud->z2xy);
    if (relSlice > 1 && relSlice < rgbs.rows - 1)
        rgbs(Range(relSlice, relSlice + 1), Range(0, IMPROJW)) = Scalar::all(150);
    int relY = IMMAINH - pos.y() - leftSag;
    if (relY > 1 && relY < rgbs.cols - 1)
        rgbs(Range(0, IMPROJH), Range(relY, relY + 1)) = Scalar::all(150);

    Scalar brd = ui->cbMarkersVisible->isChecked() ? Scalar::all(0) : Scalar(255, 0, 0);
    drawImageBorder(rgbs, brd);

    ui->lblProjSag->setPixmap(QPixmap::fromImage(QImage(rgbs.data, rgbs.cols, rgbs.rows, rgbs.step, QImage::Format_RGB888)));
}

void MainWindow::redrawSlice(){
    UserData * ud = getUserData();

    updateMainSize();

    if (im3.slices.size() > 0){
        QPoint pos(round(lastPos.rx()), round(lastPos.ry()));
        pos.setX(pos.x() * 512.0 / ud->mainSize);
        pos.setY(pos.y() * 512.0 / ud->mainSize);

        int w = ui->sldWidthHU->sliderPosition();
        int l = ui->sldLevelHU->sliderPosition() + 1024;
        double alpha = 255. / w;
        double beta = - alpha * (l - w / 2);

        drawMainImage(pos, alpha, beta);

        if (pos.x() >= 0 && pos.x() < IMMAINW && pos.y() >= 0 && pos.y() < IMMAINH){
            drawFrontalImage(pos, alpha, beta);
            drawSagitalImage(pos, alpha, beta);
        }

        ui->sldSlices->setSliderPosition(ud->curSlice);
        ui->sldToolSize->setSliderPosition(ud->toolSize);        
        updateStatusBar();
    }
    else {
        filLabelImages();
    }
    ui->sldOpacity->setEnabled(ui->cbMarkersVisible->isChecked());
}

void MainWindow::on_sldSlices_sliderMoved(int position)
{
    UserData * ud = getUserData();

    ud->curSlice = position;
    redrawSlice();
}

void MainWindow::on_sldSlices_valueChanged(int value)
{
    on_sldSlices_sliderMoved(value);
}

void MainWindow::on_cbMarkersVisible_clicked()
{
    redrawSlice();
}

void MainWindow::on_sldToolSize_sliderMoved(int position)
{
    UserData * ud = getUserData();

    ud->toolSize = position;
    redrawSlice();
}

void MainWindow::on_sldToolSize_valueChanged(int value)
{
    on_sldToolSize_sliderMoved(value);
}

void MainWindow::on_btnDispDefault_clicked()
{
    setLevelAndWidthHU(-700, 1500);
    redrawSlice();
}

void MainWindow::closeEvent (QCloseEvent *event)
{
    if (checkUnsavedChanges())
        event->accept();
    else
        event->ignore();
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, "About",
                       "Lesion Labeller by Vitali Liauchuk,\nBiomedical Image Analysis dept., UIIP NASB\n\nvitali.liauchuk@gmail.com");
}

void MainWindow::on_actionInfo_triggered()
{
    if (QFileInfo(PDFFILENAME).exists()){
        QDesktopServices::openUrl(QUrl(PDFFILENAME));
    }
    else {
        QMessageBox::warning(this, "Warning", QString("File '%1' not found.").arg(PDFFILENAME));
    }
}

void MainWindow::on_sldOpacity_sliderMoved(int position)
{
    redrawSlice();
}

void MainWindow::on_sldOpacity_valueChanged(int value)
{
    on_sldOpacity_sliderMoved(value);
}

void MainWindow::on_sldLevelHU_sliderMoved(int position)
{
    ui->lblLevelHU->setText(QString("Level: %1").arg(ui->sldLevelHU->sliderPosition()));
    redrawSlice();
}

void MainWindow::on_sldLevelHU_valueChanged(int value)
{
    on_sldLevelHU_sliderMoved(value);
}

void MainWindow::on_actionLung_700_1500_triggered()
{
    setLevelAndWidthHU(-700, 1500);
}

void MainWindow::on_actionChest_40_400_triggered()
{
    setLevelAndWidthHU(40, 400);
}

void MainWindow::on_actionBone_500_2000_triggered()
{
    setLevelAndWidthHU(500, 2000);
}

void MainWindow::on_sldWidthHU_sliderMoved(int position)
{
    ui->lblWidthHU->setText(QString("Width: %1").arg(ui->sldWidthHU->sliderPosition()));
    redrawSlice();
}

void MainWindow::on_sldWidthHU_valueChanged(int value)
{
    on_sldWidthHU_sliderMoved(value);
}

void MainWindow::updateMainSize()
{
    UserData * ud = getUserData();

    int availableWidth = this->width() - 631;
    int availableHeight = this->height() - 51;
    ud->mainSize = availableWidth > availableHeight ? availableHeight : availableWidth;
    ud->mainSize = ud->mainSize > MINMAINSZ ? ud->mainSize : MINMAINSZ;
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);

    UserData * ud = getUserData();

    updateMainSize();

    ui->gbDispParams->setGeometry(850 - 512 + ud->mainSize, ui->gbDispParams->y(), ui->gbDispParams->width(), ui->gbDispParams->height());
    ui->gbMarkers->setGeometry(850 - 512 + ud->mainSize, ui->gbMarkers->y(), ui->gbMarkers->width(), ui->gbMarkers->height());
    ui->lblProjFront->setGeometry(580 - 512 + ud->mainSize, ui->lblProjFront->y(), ui->lblProjFront->width(), ui->lblProjFront->height());
    ui->lblProjSag->setGeometry(580 - 512 + ud->mainSize, ui->lblProjSag->y(), ui->lblProjSag->width(), ui->lblProjSag->height());
    ui->sldSlices->setGeometry(530 - 512 + ud->mainSize, ui->sldSlices->y(), ui->sldSlices->width(), ud->mainSize);
    ui->lblSliceLabels->setGeometry(550 - 512 + ud->mainSize, ui->lblSliceLabels->y(), ui->lblSliceLabels->width(), ud->mainSize);
    ui->lblMainImage->setGeometry(10, 10, ud->mainSize, ud->mainSize);

    ui->gbWait->setGeometry((this->width() - ui->gbWait->width()) / 2, (this->height() - ui->gbWait->height()) / 3, ui->gbWait->width(), ui->gbWait->height());

    redrawSlice();
}

void MainWindow::on_actionSmall_triggered()
{
    ui->sldToolSize->setSliderPosition(4);
}

void MainWindow::on_actionMedium_triggered()
{
    ui->sldToolSize->setSliderPosition(7);
}

void MainWindow::on_actionLarge_triggered()
{
    ui->sldToolSize->setSliderPosition(12);
}

void MainWindow::on_actionMaximum_triggered()
{
    ui->sldToolSize->setSliderPosition(ui->sldToolSize->maximum());
}

void MainWindow::on_btnUpdateProj_clicked()
{
    if (im3.slices.size() > 0){
        ui->lblWaitMsg->setText("Updating views ...");
        ui->gbWait->show();
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        updateFrontalAndSagitalLabels();
        recalcSliceLabels();

        ui->gbWait->hide();
    }
}

void MainWindow::on_sldThres_sliderMoved(int position)
{
    ui->cbThresholded->setText(QString("(T) Thresholded (%1)").arg(ui->sldThres->value()));
}

void MainWindow::on_sldThres_valueChanged(int value)
{
    on_sldThres_sliderMoved(value);
}

void MainWindow::on_actionExport_triggered()
{
    UserData * ud = getUserData();

    if (!im3.slices.empty()){
        ui->lblWaitMsg->setText("Exporting as JPEG ...");
        ui->gbWait->show();
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        int curSlice = ud->curSlice;

        int w = ui->sldWidthHU->sliderPosition();
        int l = ui->sldLevelHU->sliderPosition() + 1024;
        double alpha = 255. / w;
        double beta = - alpha * (l - w / 2);

        QString origFilepath(ud->filePath);
        LOG(QString("Exporting labelling for '").append(origFilepath).append("'.").toLatin1().data());

        QString dirPath = origFilepath.replace(".hdr", "");

        if (!QDir(dirPath).exists())
            QDir().mkdir(dirPath);

        for (int k = 0; k < im3.slices.size(); k++){
            ud->curSlice = k;
            Mat rgb = composeMainImage(QPoint(-100, -100), alpha, beta, ui);

            std::vector<Mat> channels;
            Mat tmp;
            split(rgb, channels);
            tmp = channels[2];
            channels[2] = channels[0];
            channels[0] = tmp;
            merge(channels, rgb);

            QString filePath = QString().sprintf("%s/labels_%03i.jpg", dirPath.toLatin1().data(), k);
            imwrite(filePath.toLatin1().data(), rgb);
        }

        ud->curSlice = curSlice;
        statusBar()->showMessage("Export complete");

        ui->gbWait->hide();
    }
}

void MainWindow::recalcSliceLabels(){
    UserData * ud = getUserData();

    sliceLabels = Mat(lbl.slices.size(), 10, CV_8UC3, Scalar::all(255));

    for (int k = 0; k < lbl.slices.size(); k++){
        Mat l = lbl.slices[lbl.slices.size() - k - 1];

        for (int i = 0; i < l.rows; i++){
            for (int j = 0; j < l.cols; j++){
                int label = l.at<uchar>(i, j);
                if (label > 0)
                    sliceLabels.at<Vec3b>(k, label - 1) = ud->palette[label];
            }
        }
    }
}

void MainWindow::on_cbOrigSliceLabels_clicked()
{
    redrawSlice();
}
