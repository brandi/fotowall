/***************************************************************************
 *                                                                         *
 *   This file is part of the Fotowall project,                            *
 *       http://www.enricoros.com/opensource/fotowall                      *
 *                                                                         *
 *   Copyright (C) 2007-2009 by Enrico Ros <enrico.ros@gmail.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "App/MainWindow.h"

#include "3rdparty/likebackfrontend/LikeBack.h"
#include "Canvas/Canvas.h"
#include "Shared/ButtonsDialog.h"
#include "Shared/MetaXmlReader.h"
#include "Shared/RenderOpts.h"
#include "Shared/VideoProvider.h"
#include "App.h"
#include "ExactSizeDialog.h"
#include "ExportWizard.h"
#include "ModeInfo.h"
#include "SceneView.h"
#include "Settings.h"
#include "VersionCheckDialog.h"
#include "XmlRead.h"
#include "XmlSave.h"
#include "ui_MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QDesktopWidget>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFile>
#include <QImageReader>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QProgressDialog>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include "math.h"

// current location and 'check string' for the tutorial
#define TUTORIAL_URL QUrl("http://fosswire.com/post/2008/09/fotowall-make-wallpaper-collages-from-your-photos/")
#define TUTORIAL_STRING "Peter walks you through how to use Foto"
#define ENRICOBLOG_STRING "http://www.enricoros.com/blog/tag/fotowall/"
#define FOTOWALL_FEEDBACK_LANGS "en,it,fr"
#define FOTOWALL_FEEDBACK_SERVER "www.enricoros.com"
#define FOTOWALL_FEEDBACK_PATH "/opensource/fotowall/feedback/send.php"

#define REQUIRE_CANVAS \
    if (!m_canvas) return;
#define REQUIRE_CANVAS_R(value) \
    if (!m_canvas) return value;


MainWindow::MainWindow(const QStringList & contentUrls, QWidget * parent)
    : QWidget(parent)
    , ui(new Ui::MainWindow())
    , m_canvas(0)
    , m_aHelpTutorial(0)
    , m_aHelpSupport(0)
    , m_gBackActions(0)
    , m_gBackRatioActions(0)
    , m_likeBack(0)
{
    // setup widget
    QRect geom = QApplication::desktop()->availableGeometry();
    resize(2 * geom.width() / 3, 2 * geom.height() / 3);
#if QT_VERSION >= 0x040500
    setWindowTitle(QCoreApplication::applicationName() + " " + QCoreApplication::applicationVersion());
#else
    setWindowTitle(QCoreApplication::applicationName() + " " + QCoreApplication::applicationVersion() + "   -Limited Edition (Qt 4.4)-");
#endif
    setWindowIcon(QIcon(":/data/fotowall.png"));

    // init ui
    ui->setupUi(this);
    ui->sceneView->setFocus();
    ui->b1->setDefaultAction(ui->aAddPicture);
    ui->b2->setDefaultAction(ui->aAddText);
    ui->b3->setDefaultAction(ui->aAddWebcam);
    ui->b4->setDefaultAction(ui->aAddFlickr);
    ui->b5->setDefaultAction(ui->aAddCanvas);
#if QT_VERSION >= 0x040500
    ui->transpBox->setEnabled(true);
    ui->accelBox->setEnabled(ui->sceneView->supportsOpenGL());
#endif
    ui->widgetProperties->collapse();
    ui->widgetCanvas->expand();

    // attach menus
    ui->arrangeButton->setMenu(createArrangeMenu());
    ui->backButton->setMenu(createBackgroundMenu());
    ui->decoButton->setMenu(createDecorationMenu());
    ui->onlineHelpButton->setMenu(createOnlineHelpMenu());

    // react to VideoProvider
    ui->aAddWebcam->setVisible(VideoProvider::instance()->inputCount() > 0);
    connect(VideoProvider::instance(), SIGNAL(inputCountChanged(int)), this, SLOT(slotVerifyVideoInputs(int)));

    // create misc actions
    createMiscActions();

    // set the startup project mode
    on_projectType_activated(0);
    m_modeInfo.setCanvasDpi(ui->sceneView->logicalDpiX(), ui->sceneView->logicalDpiY());
    m_modeInfo.setPrintDpi(300);

    // check stuff on the net
    checkForTutorial();
    checkForSupport();
    checkForUpdates();

    // setup likeback
    createLikeBack();

    // initial behavior: loaded Canvas
    if (contentUrls.size() == 1 && App::isFotowallFile(contentUrls.first())) {
        // create a custom canvas and load content over it
        Canvas * initialCanvas = new Canvas(this);
        stackCanvas(initialCanvas);
        XmlRead::read(contentUrls.first(), this, initialCanvas);
    }
    // initial behavior: new Canvas with contents
    else if (!contentUrls.isEmpty()) {
        // create a custom canvas add contents to it
        Canvas * initialCanvas = new Canvas(this);
        stackCanvas(initialCanvas);
        initialCanvas->addPictureContent(contentUrls);
    }
    // initial behavior: show the selection Scene
    else {
        Canvas * initialCanvas = new Canvas(this);
        stackCanvas(initialCanvas);
        QList<QUrl> historyUrls = App::settings->recentFotowallUrls();
/*        if (!historyUrls.isEmpty()) {
            int dCount = historyUrls.size();
            int dCols = 1 + (int)sqrt((double)dCount);
            int dRows = 1 + dCount / dCols;
#warning FROM HERE
            //int dWidth = (640 - 2*20) / ()
            int cIdx = 0;
            int rIdx = 0;
            foreach (const QUrl & url, historyUrls) {
                m_canvas->addCanvasViewContent(QStringList() << url.toString());
                // FIXME: temp, to limit to 1
                break;
            }
        }*/
    }

    // show initially
    showMaximized();
}

MainWindow::~MainWindow()
{
    // dump current layout
    if (m_canvas) {
        // this is an example of 'autosave-like function'
        //QString tempPath = QDir::tempPath() + QDir::separator() + "autosave.fotowall";
        //XmlSave::save(tempPath, m_canvas, m_canvas->projectMode(), m_modeInfo);
    }

    // delete everything
    delete m_likeBack;
    delete m_canvas;
    delete ui;
}

// TEMP
void MainWindow::stackCanvas(Canvas * newCanvas)
{
    // skip if already set
    if (newCanvas == m_canvas)
        return;

    if (m_canvas)
        disconnect(m_canvas, 0, this, 0);
    m_canvas = newCanvas;
    connect(m_canvas, SIGNAL(backModeChanged()), this, SLOT(slotBackModeChanged()));
    connect(m_canvas, SIGNAL(showPropertiesWidget(QWidget*)), this, SLOT(slotShowPropertiesWidget(QWidget*)));
    ui->sceneView->setScene(m_canvas);
    update();

    // update breadcrumb
    static quint32 baseId = 0;
    int nextId = baseId + 1;
    ui->canvasNavBar->addNode(nextId, "test", baseId++);
}

void MainWindow::setModeInfo(ModeInfo modeInfo)
{
    m_modeInfo = modeInfo;
    m_modeInfo.setCanvasDpi(ui->sceneView->logicalDpiX(), ui->sceneView->logicalDpiY());
}

ModeInfo MainWindow::getModeInfo()
{
    return m_modeInfo;
}

void MainWindow::restoreMode(int mode)
{
    if (mode == 3) { // If exact size project
        // Called here not to have the unneeded size dialog
        setExactSizeProject();
    } else {
        on_projectType_activated(mode);
    }
}

void MainWindow::showIntroduction()
{
    if (m_canvas)
        m_canvas->showIntroduction();
}

void MainWindow::closeEvent(QCloseEvent * event)
{
    // build the closure dialog
    ButtonsDialog quitAsk("MainWindow-Exit", tr("Closing Fotowall..."));
    quitAsk.setMinimumWidth(350);
    quitAsk.setButtonText(QDialogButtonBox::Cancel, tr("Cancel"));
    if (m_canvas && m_canvas->pendingChanges()) {
        quitAsk.setMessage(tr("Are you sure you want to quit and lose your changes?"));
        quitAsk.setButtonText(QDialogButtonBox::Save, tr("Save"));
        quitAsk.setButtonText(QDialogButtonBox::Close, tr("Don't Save"));
        quitAsk.setButtons(QDialogButtonBox::Save | QDialogButtonBox::Close | QDialogButtonBox::Cancel);
    } else {
        quitAsk.setMessage(tr("Are you sure you want to quit?"));
        quitAsk.setButtonText(QDialogButtonBox::Close, tr("Quit"));
        quitAsk.setButtons(QDialogButtonBox::Close | QDialogButtonBox::Cancel);
    }

    // react to the dialog's answer
    switch (quitAsk.execute()) {
        case QDialogButtonBox::Cancel:
            event->ignore();
            break;

        case QDialogButtonBox::Save:
            // save file and return to Fotowall if canceled
            if (!on_saveButton_clicked()) {
                event->ignore();
                break;
            }
            // fall through

        default:
            event->accept();
            break;
    }
}

QMenu * MainWindow::createArrangeMenu()
{
    QMenu * menu = new QMenu();

    QAction * aForceField = new QAction(tr("Enable force field"), menu);
    aForceField->setCheckable(true);
    if (m_canvas) // FIXME: reflect a property
        aForceField->setChecked(m_canvas->forceFieldEnabled());
    connect(aForceField, SIGNAL(toggled(bool)), this, SLOT(slotArrangeForceField(bool)));
    menu->addAction(aForceField);

    QAction * aNP = new QAction(tr("Auto-arrange new pictures"), menu);
    aNP->setCheckable(true);
    aNP->setChecked(false);
    //connect(aNP, SIGNAL(toggled(bool)), this, SLOT(slotArrangeNew(bool)));
    menu->addAction(aNP);

    menu->addSeparator()->setText(tr("Rearrange"));

    QAction * aAU = new QAction(tr("Random"), menu);
    connect(aAU, SIGNAL(triggered()), this, SLOT(slotArrangeRandom()));
    menu->addAction(aAU);

    QAction * aAS = new QAction(tr("Shaped"), menu);
    aAS->setEnabled(false);
    //connect(aAS, SIGNAL(triggered()), this, SLOT(slotArrangeShape()));
    menu->addAction(aAS);

    QAction * aAC = new QAction(tr("Collage"), menu);
    aAC->setEnabled(false);
    //connect(aAC, SIGNAL(triggered()), this, SLOT(slotArrangeCollage()));
    menu->addAction(aAC);

    return menu;
}

QMenu * MainWindow::createBackgroundMenu()
{
    QMenu * menu = new QMenu();
    m_gBackActions = new QActionGroup(menu);
    connect(m_gBackActions, SIGNAL(triggered(QAction*)), this, SLOT(slotSetBackMode(QAction*)));

    QAction * aNone = new QAction(tr("None"), menu);
    aNone->setToolTip(tr("Transparency can be saved to PNG images only."));
    aNone->setProperty("id", 1);
    aNone->setCheckable(true);
    aNone->setActionGroup(m_gBackActions);
    menu->addAction(aNone);

    QAction * aGradient = new QAction(tr("Gradient"), menu);
    aGradient->setProperty("id", 2);
    aGradient->setCheckable(true);
    aGradient->setActionGroup(m_gBackActions);
    menu->addAction(aGradient);

    QAction * aContent = new QAction(tr("Content"), menu);
    aContent->setToolTip(tr("Double click on any content to put it on background."));
    aContent->setEnabled(false);
    aContent->setProperty("id", 3);
    aContent->setCheckable(true);
    aContent->setActionGroup(m_gBackActions);
    menu->addAction(aContent);

    menu->addSeparator();

    QMenu * mScaling = new QMenu(tr("Content Aspect Ratio"), menu);
    m_gBackRatioActions = new QActionGroup(menu);
    connect(m_gBackRatioActions, SIGNAL(triggered(QAction*)), this, SLOT(slotSetBackRatio(QAction*)));
    menu->addMenu(mScaling);

    QAction * aRatioKeepEx = new QAction(tr("Keep proportions by expanding"), mScaling);
    aRatioKeepEx->setProperty("mode", (int)Qt::KeepAspectRatioByExpanding);
    aRatioKeepEx->setCheckable(true);
    aRatioKeepEx->setActionGroup(m_gBackRatioActions);
    mScaling->addAction(aRatioKeepEx);

    QAction * aRatioKeep = new QAction(tr("Keep proportions"), mScaling);
    aRatioKeep->setProperty("mode", (int)Qt::KeepAspectRatio);
    aRatioKeep->setCheckable(true);
    aRatioKeep->setActionGroup(m_gBackRatioActions);
    mScaling->addAction(aRatioKeep);

    QAction * aRatioIgnore = new QAction(tr("Ignore proportions"), mScaling);
    aRatioIgnore->setProperty("mode", (int)Qt::IgnoreAspectRatio);
    aRatioIgnore->setCheckable(true);
    aRatioIgnore->setActionGroup(m_gBackRatioActions);
    mScaling->addAction(aRatioIgnore);

    // initially check the action
    slotBackModeChanged();
    slotBackRatioChanged();
    return menu;
}

QMenu * MainWindow::createDecorationMenu()
{
    QMenu * menu = new QMenu();

    QAction * aTop = new QAction(tr("Top bar"), menu);
    aTop->setCheckable(true);
    if (m_canvas) // FIXME: bind to a property
        aTop->setChecked(m_canvas->topBarEnabled());
    connect(aTop, SIGNAL(toggled(bool)), this, SLOT(slotDecoTopBar(bool)));
    menu->addAction(aTop);

    QAction * aBottom = new QAction(tr("Bottom bar"), menu);
    aBottom->setCheckable(true);
    if (m_canvas) // FIXME: bind to a property
        aBottom->setChecked(m_canvas->bottomBarEnabled());
    connect(aBottom, SIGNAL(toggled(bool)), this, SLOT(slotDecoBottomBar(bool)));
    menu->addAction(aBottom);

    menu->addSeparator();

    QAction * aSetTitle = new QAction(tr("Set title..."), menu);
    connect(aSetTitle, SIGNAL(triggered()), this, SLOT(slotDecoSetTitle()));
    menu->addAction(aSetTitle);

    QAction * aClearTitle = new QAction(tr("Clear title"), menu);
    connect(aClearTitle, SIGNAL(triggered()), this, SLOT(slotDecoClearTitle()));
    menu->addAction(aClearTitle);

    return menu;
}

QMenu * MainWindow::createOnlineHelpMenu()
{
    QMenu * menu = new QMenu();
    menu->setSeparatorsCollapsible(false);

    m_aHelpTutorial = new QAction(tr("Tutorial Video (0.2)"), menu);
    connect(m_aHelpTutorial, SIGNAL(triggered()), this, SLOT(slotHelpTutorial()));
    menu->addAction(m_aHelpTutorial);

    QAction * aCheckUpdates = new QAction(tr("Check for Updates"), menu);
    connect(aCheckUpdates, SIGNAL(triggered()), this, SLOT(slotHelpUpdates()));
    menu->addAction(aCheckUpdates);

    QAction * aFotowallBlog = new QAction(tr("Fotowall's Blog"), menu);
    connect(aFotowallBlog, SIGNAL(triggered()), this, SLOT(slotHelpWebsite()));
    menu->addAction(aFotowallBlog);

    m_aHelpSupport = new QAction("", menu);
    connect(m_aHelpSupport, SIGNAL(triggered()), this, SLOT(slotHelpSupport()));
    menu->addAction(m_aHelpSupport);

    return menu;
}

void MainWindow::createLikeBack()
{
    m_likeBack = new LikeBack(LikeBack::AllButtons, false, this);
    m_likeBack->setAcceptedLanguages(QString(FOTOWALL_FEEDBACK_LANGS).split(","));
    m_likeBack->setServer(FOTOWALL_FEEDBACK_SERVER, FOTOWALL_FEEDBACK_PATH);
}

void MainWindow::createMiscActions()
{
    // select all
    QAction * aSA = new QAction(tr("Select all"), this);
    aSA->setShortcut(tr("CTRL+A"));
    connect(aSA, SIGNAL(triggered()), this, SLOT(slotActionSelectAll()));
    addAction(aSA);
}

void MainWindow::checkForTutorial()
{
    // hide the tutorial link
    m_aHelpTutorial->setVisible(false);

    // try to get the tutorial page (note, multiple QNAMs will be deleted on app closure)
    QNetworkAccessManager * manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(slotVerifyTutorial(QNetworkReply*)));
    manager->get(QNetworkRequest(TUTORIAL_URL));
}

void MainWindow::checkForSupport()
{
    // hide the support link
    m_aHelpSupport->setVisible(false);

    // check the Open Collaboration Services knowledgebase for Fotowall
    QTimer::singleShot(2000, this, SLOT(slotVerifySupport()));
}

void MainWindow::checkForUpdates()
{
    // find out the time of the last update check
    QDate lastCheck = App::settings->value("Fotowall/LastUpdateCheck").toDate();
    if (lastCheck.isNull()) {
        App::settings->setValue("Fotowall/LastUpdateCheck", QDate::currentDate());
        return;
    }

    // check for updates 30 days after the last one
    if (lastCheck.daysTo(QDate::currentDate()) > 30)
        QTimer::singleShot(2000, this, SLOT(slotHelpUpdates()));
}

void MainWindow::setNormalProject()
{
    m_modeInfo.setRealSizeInches(-1,-1); // Unset the size (for the saving function)
    static bool skipFirstMaximizeHack = true;
    ui->sceneView->setMinimumSize(ui->sceneView->minimumSizeHint());
    ui->sceneView->setMaximumSize(QSize(16777215, 16777215));
    if (skipFirstMaximizeHack)
        skipFirstMaximizeHack = false;
    else
        showMaximized();
    ui->exportButton->setText(tr("Export"));
    if (m_canvas) // FIXME: bind to a property
        m_canvas->setProjectMode(Canvas::ModeNormal);
    ui->projectType->setCurrentIndex(0);
}

void MainWindow::setCDProject()
{
    // A CD cover is a 4.75x4.715 inches square.
    m_modeInfo.setRealSizeInches(4.75, 4.75);
    m_modeInfo.setLandscape(false);
    ui->sceneView->setFixedSize(m_modeInfo.canvasPixelSize());
    showNormal();
    ui->exportButton->setText(tr("print"));
    if (m_canvas) // FIXME: bind to a property
        m_canvas->setProjectMode(Canvas::ModeCD);
    ui->projectType->setCurrentIndex(1);
}

void MainWindow::setDVDProject()
{
    m_modeInfo.setRealSizeInches((float)10.83, (float)7.2);
    m_modeInfo.setLandscape(true);
    ui->sceneView->setFixedSize(m_modeInfo.canvasPixelSize());
    showNormal();
    ui->exportButton->setText(tr("print"));
    if (m_canvas) // FIXME: bind to a property
        m_canvas->setProjectMode(Canvas::ModeDVD);
    ui->projectType->setCurrentIndex(2);
}

void MainWindow::setExactSizeProject()
{
    // Exact size mode
    if(m_modeInfo.realSize().isEmpty()) {
        ExactSizeDialog sizeDialog;
        QPointF screenDpi = m_modeInfo.canvasDpi();
        if (screenDpi.x() == screenDpi.y())
            sizeDialog.ui.screenDpi->setValue(screenDpi.x());
        else
            sizeDialog.ui.screenDpi->setSpecialValueText(QString("%1, %2").arg(screenDpi.x()).arg(screenDpi.y()));
        if(sizeDialog.exec() != QDialog::Accepted) {
            return;
        }
        float w = sizeDialog.ui.widthSpinBox->value();
        float h = sizeDialog.ui.heightSpinBox->value();
        int printDpi = sizeDialog.ui.printDpi->value();
        bool landscape = sizeDialog.ui.landscapeCheckBox->isChecked();
        m_modeInfo.setLandscape(landscape);
        m_modeInfo.setPrintDpi(printDpi);
        if(sizeDialog.ui.unityComboBox->currentIndex() == 0)
            m_modeInfo.setRealSizeCm(w, h);
        else
            m_modeInfo.setRealSizeInches(w, h);
    }
    ui->sceneView->setFixedSize(m_modeInfo.canvasPixelSize());
    showNormal();
    ui->exportButton->setText(tr("print"));
    if (m_canvas) // FIXME: bind to a property
        m_canvas->setProjectMode(Canvas::ModeExactSize);
    ui->projectType->setCurrentIndex(3);
}

void MainWindow::on_projectType_activated(int index)
{
    m_modeInfo.setRealSizeInches(-1,-1); // Unset the size (so if it is a mode that require
                                        // asking size, it will be asked !
    switch (index) {
        case 0: //Normal project
            setNormalProject();
            break;

        case 1: // CD cover
            setCDProject();
            break;

        case 2: //DVD cover
            setDVDProject();
            break;

        case 3: //Exact Size
            setExactSizeProject();
            break;
    }
}

void MainWindow::on_aAddCanvas_triggered()
{
    REQUIRE_CANVAS
    // make up the default load path (stored as 'Fotowall/LoadProjectDir')
    QString defaultLoadPath = App::settings->value("Fotowall/LoadProjectDir").toString();

    // ask the file name, validate it, store back to settings and load the file
    QStringList fileNames = QFileDialog::getOpenFileNames(ui->sceneView, tr("Select one or more Fotowall files to add"), defaultLoadPath, tr("Fotowall (*.fotowall)"));
    if (fileNames.isEmpty())
        return;
    App::settings->setValue("Fotowall/LoadProjectDir", QFileInfo(fileNames[0]).absolutePath());
    m_canvas->addCanvasViewContent(fileNames);
}

void MainWindow::on_aAddFlickr_toggled(bool on)
{
    REQUIRE_CANVAS
    m_canvas->setWebContentSelectorVisible(on);
}

void MainWindow::on_aAddPicture_triggered()
{
    REQUIRE_CANVAS
    // build the extensions list
    QString extensions;
    foreach (const QByteArray & format, QImageReader::supportedImageFormats())
        extensions += "*." + format + " *." + format.toUpper() + " ";

    // make up the default load path (stored as 'Fotowall/LoadImagesDir')
    QString defaultLoadPath = App::settings->value("Fotowall/LoadImagesDir").toString();

    // ask the file name, validate it, store back to settings and load the file
    QStringList fileNames = QFileDialog::getOpenFileNames(ui->sceneView, tr("Select one or more pictures to add"), defaultLoadPath, tr("Images (%1)").arg(extensions));
    if (fileNames.isEmpty())
        return;    
    App::settings->setValue("Fotowall/LoadImagesDir", QFileInfo(fileNames[0]).absolutePath());
    m_canvas->addPictureContent(fileNames);
}

void MainWindow::on_aAddText_triggered()
{
    REQUIRE_CANVAS
    m_canvas->addTextContent();
}

void MainWindow::on_aAddWebcam_triggered()
{
    REQUIRE_CANVAS
    m_canvas->addWebcamContent(0);
}

void MainWindow::on_accelBox_toggled(bool enabled)
{
    // ask for confirmation when enabling opengl
    if (enabled) {
        ButtonsDialog warning("GoOpenGL", tr("OpenGL"), tr("OpenGL accelerates graphics. However it's not guaranteed that it will work on your system.<br>Just try and see if it works for you ;-)<br> - if it feels slower, make sure that your driver accelerates OpenGL<br> - if Fotowall stops responding after switching to OpenGL, just don't use this feature next time<br><br>NOTE: OpenGL doesn't work with 'Transparent' mode.<br>"), QDialogButtonBox::Ok | QDialogButtonBox::Cancel, true, true);
        warning.setIcon(QStyle::SP_MessageBoxInformation);
        if (warning.execute() == QDialogButtonBox::Cancel) {
            ui->accelBox->setChecked(false);
            return;
        }

        // toggle transparency with opengl
        ui->transpBox->setChecked(false);
    }

    // set opengl state
    ui->sceneView->setOpenGL(enabled);
}

#ifdef Q_WS_WIN
/**
  Blur behind windows (on Windows Vista/7)

  The following code snippet has been borrowed from Jens of Qt Software / Nokia
  see: http://labs.qt.nokia.com/blogs/2009/09/15/using-blur-behind-on-windows/
  the license says: Use, modification and distribution is allowed without
  limitation, warranty, liability or support of any kind.
**/
#include <QLibrary>
#include <qt_windows.h>

// Dwm Data Structures
#define DWM_BB_ENABLE                 0x00000001  // fEnable has been specified
typedef struct _DWM_BLURBEHIND
{
    DWORD dwFlags;
    BOOL fEnable;
    HRGN hRgnBlur;
    BOOL fTransitionOnMaximized;
} DWM_BLURBEHIND;

// Dwm entry points
typedef HRESULT (WINAPI *PtrDwmIsCompositionEnabled)(BOOL * pfEnabled);
typedef HRESULT (WINAPI *PtrDwmEnableBlurBehindWindow)(HWND hWnd, const DWM_BLURBEHIND * pBlurBehind);
static PtrDwmIsCompositionEnabled pDwmIsCompositionEnabled = 0;
static PtrDwmEnableBlurBehindWindow pDwmEnableBlurBehindWindow  = 0;

static bool dwmResolveLibs()
{
    if (!pDwmIsCompositionEnabled) {
        QLibrary dwmLib(QString::fromAscii("dwmapi"));
        pDwmIsCompositionEnabled = (PtrDwmIsCompositionEnabled)dwmLib.resolve("DwmIsCompositionEnabled");
        pDwmEnableBlurBehindWindow = (PtrDwmEnableBlurBehindWindow)dwmLib.resolve("DwmEnableBlurBehindWindow");
    }
    return pDwmIsCompositionEnabled != 0;
}

static bool dwmEnableBlurBehindWindow(QWidget * widget, bool enable)
{
    bool result = false;
    if (dwmResolveLibs()) {
        DWM_BLURBEHIND bb = {0};
        bb.dwFlags = DWM_BB_ENABLE;
        bb.fEnable = enable;
        bb.hRgnBlur = NULL;
        HRESULT hr = pDwmEnableBlurBehindWindow(widget->winId(), &bb);
        if (SUCCEEDED(hr))
            result = true;
    }
    return result;
}
#endif

void MainWindow::on_transpBox_toggled(bool transparent)
{
#if QT_VERSION >= 0x040500
    static Qt::WindowFlags initialWindowFlags = windowFlags();
    if (transparent) {
        // one-time warning
        ButtonsDialog warning("GoTransparent", tr("Transparency"), tr("This feature has not been widely tested yet.<br> - on linux it requires compositing (like compiz/beryl, kwin4)<br> - on windows and mac it seems to work<br>If you see a black background then transparency is not supported on your system.<br><br>NOTE: you should set the 'Transparent' Background to notice the the window transparency.<br>"), QDialogButtonBox::Ok, true, true);
        warning.setIcon(QStyle::SP_MessageBoxInformation);
        warning.execute();

        // go transparent
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_TranslucentBackground, true);

        // hint the render that we're transparent now
        RenderOpts::ARGBWindow = true;

#ifdef Q_OS_WIN
        // enable blur behind on Vista/7
        if (!dwmEnableBlurBehindWindow(this, true)) {
            // if blur fails, use a frameless window that's needed on XP for transparency
            setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
            show();
        }
#endif

        // set 'NoBackground' to show that we're transparent for real
        if (m_canvas) // FIXME: bind to a property
            m_canvas->setBackMode(1);
    } else {
        // back to normal (non-alphaed) window
        setAttribute(Qt::WA_TranslucentBackground, false);
        setAttribute(Qt::WA_NoSystemBackground, false);

#ifdef Q_OS_WIN
        // disable no-border on windows
        setWindowFlags(initialWindowFlags);
        show();
#endif

        // hint the render that we're opaque again
        RenderOpts::ARGBWindow = false;
    }
    // refresh the window
    update();
#else
    Q_UNUSED(transparent)
#endif
}

void MainWindow::on_introButton_clicked()
{
    REQUIRE_CANVAS
    m_canvas->showIntroduction();
}

void MainWindow::on_lbLike_clicked()
{
    m_likeBack->execCommentDialog(LikeBack::Like);
}

void MainWindow::on_lbDislike_clicked()
{
    m_likeBack->execCommentDialog(LikeBack::Dislike);
}

void MainWindow::on_lbFeature_clicked()
{
    m_likeBack->execCommentDialog(LikeBack::Feature);
}

void MainWindow::on_lbBug_clicked()
{
    m_likeBack->execCommentDialog(LikeBack::Bug);
}

bool MainWindow::on_loadButton_clicked()
{
    REQUIRE_CANVAS_R(false)

    // make up the default load path (stored as 'Fotowall/LoadProjectDir')
    QString defaultLoadPath = App::settings->value("Fotowall/LoadProjectDir").toString();

    // ask the file name, validate it, store back to settings and load the file
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select the Fotowall file"), defaultLoadPath, tr("Fotowall (*.fotowall)"));
    if (fileName.isNull())
        return false;
    App::settings->setValue("Fotowall/LoadProjectDir", QFileInfo(fileName).absolutePath());
    return XmlRead::read(fileName, this, m_canvas);
}

bool MainWindow::on_saveButton_clicked()
{
    REQUIRE_CANVAS_R(false)

    // make up the default save path (stored as 'Fotowall/SaveProjectDir')
    QString defaultSavePath = tr("Unnamed %1.fotowall").arg(QDate::currentDate().toString());
    if (App::settings->contains("Fotowall/SaveProjectDir"))
        defaultSavePath.prepend(App::settings->value("Fotowall/SaveProjectDir").toString() + QDir::separator());

    // ask the file name, validate it, store back to settings and save over it
    QString fileName = QFileDialog::getSaveFileName(this, tr("Select the Fotowall file"), defaultSavePath, "Fotowall (*.fotowall)");
    if (fileName.isNull())
        return false;
    App::settings->setValue("Fotowall/SaveProjectDir", QFileInfo(fileName).absolutePath());
    if (!fileName.endsWith(".fotowall", Qt::CaseInsensitive))
        fileName += ".fotowall";
    return XmlSave::save(fileName, m_canvas, m_canvas->projectMode(), m_modeInfo);
}

void MainWindow::on_exportButton_clicked()
{
    REQUIRE_CANVAS
    // show the Export Wizard on normal mode
    if (m_canvas->projectMode() == Canvas::ModeNormal) {
        ExportWizard(m_canvas).exec();
        return;
    }

    // print on other modes
    m_canvas->printAsImage(m_modeInfo.printDpi(), m_modeInfo.printPixelSize(), m_modeInfo.landscape());
}
/*
void MainWindow::on_quitButton_clicked()
{
    QCoreApplication::quit();
}
*/
void MainWindow::slotActionSelectAll()
{
    REQUIRE_CANVAS
    m_canvas->selectAllContent();
}

void MainWindow::slotArrangeForceField(bool checked)
{
    REQUIRE_CANVAS
    m_canvas->setForceFieldEnabled(checked);
}

#include "Canvas/AbstractContent.h"
void MainWindow::slotArrangeRandom()
{
    REQUIRE_CANVAS
    QRectF r = m_canvas->sceneRect();
    foreach (QGraphicsItem * item, m_canvas->items()) {
        AbstractContent * content = dynamic_cast<AbstractContent *>(item);
        if (!content)
            continue;
        content->setPos(r.left() + (qrand() % (int)r.width()), r.top() + (qrand() % (int)r.height()));
        content->setRotation(-30 + (qrand() % 60), Qt::ZAxis);
#if QT_VERSION >= 0x040500
        content->setOpacity((qreal)(qrand() % 100) / 99.0);
#endif
    }
}

void MainWindow::slotDecoTopBar(bool checked)
{
    REQUIRE_CANVAS
    m_canvas->setTopBarEnabled(checked);
}

void MainWindow::slotDecoBottomBar(bool checked)
{
    REQUIRE_CANVAS
    m_canvas->setBottomBarEnabled(checked);
}

void MainWindow::slotDecoSetTitle()
{
    REQUIRE_CANVAS
    // set a dummy title, if none
    if (m_canvas->titleText().isEmpty())
        m_canvas->setTitleText("...");

    // change title dialog
    bool ok = false;
    QString title = QInputDialog::getText(0, tr("Title"), tr("Insert the title"), QLineEdit::Normal, m_canvas->titleText(), &ok);
    if (ok)
        m_canvas->setTitleText(title);
}

void MainWindow::slotDecoClearTitle()
{
    REQUIRE_CANVAS
    m_canvas->setTitleText(QString());
}

void MainWindow::slotHelpWebsite()
{
    // start a fetch if no URL has been determined
    if (m_website.isEmpty()) {
        MetaXml::Connector * conn = new MetaXml::Connector();
        connect(conn, SIGNAL(fetched()), this, SLOT(slotHelpWebsiteFetched()));
        connect(conn, SIGNAL(fetchError(const QString &)), this, SLOT(slotHelpWebsiteFetchError()));
        return;
    }

    // open the website
    int answer = QMessageBox::question(this, tr("Opening Fotowall's author Blog"), tr("This is the blog of the main author of Fotowall.\nYou can find some news while we set up a proper website ;-)\nDo you want to open the web page?"), QMessageBox::Yes, QMessageBox::No);
    if (answer == QMessageBox::Yes)
        QDesktopServices::openUrl(QUrl(m_website));
}

void MainWindow::slotHelpWebsiteFetched()
{
    // get the websites from the conn
    MetaXml::Connector * conn = dynamic_cast<MetaXml::Connector *>(sender());
    if (conn && !conn->reader()->websites.isEmpty()) {
        m_website = conn->reader()->websites.first().url;
        if (!m_website.isEmpty()) {
            slotHelpWebsite();
            return;
        }
    }

    // catch-all condition: use default url
    slotHelpWebsiteFetchError();
}

void MainWindow::slotHelpWebsiteFetchError()
{
    m_website = ENRICOBLOG_STRING;
    slotHelpWebsite();
}

void MainWindow::slotHelpSupport()
{
}

void MainWindow::slotHelpTutorial()
{
    int answer = QMessageBox::question(this, tr("Opening the Web Tutorial"), tr("The Tutorial is provided on Fosswire by Peter Upfold.\nIt's about Fotowall 0.2 a rather old version.\nDo you want to open the web page?"), QMessageBox::Yes, QMessageBox::No);
    if (answer == QMessageBox::Yes)
        QDesktopServices::openUrl(TUTORIAL_URL);
}

void MainWindow::slotHelpUpdates()
{
    VersionCheckDialog vcd;
    vcd.exec();
    App::settings->setValue("Fotowall/LastUpdateCheck", QDate::currentDate());
}

void MainWindow::slotSetBackMode(QAction* action)
{
    REQUIRE_CANVAS
    int choice = action->property("id").toUInt();
    m_canvas->setBackMode(choice);
}

void MainWindow::slotSetBackRatio(QAction* action)
{
    REQUIRE_CANVAS
    Qt::AspectRatioMode mode = (Qt::AspectRatioMode)action->property("mode").toInt();
    m_canvas->setBackContentRatio(mode);
}

void MainWindow::slotBackModeChanged()
{
    REQUIRE_CANVAS
    int mode = m_canvas->backMode();
    m_gBackActions->actions()[mode - 1]->setChecked(true);
    m_gBackActions->actions()[2]->setEnabled(mode == 3);
}

void MainWindow::slotBackRatioChanged()
{
    REQUIRE_CANVAS
    Qt::AspectRatioMode mode = m_canvas->backContentRatio();
    if (mode == Qt::KeepAspectRatioByExpanding)
        m_gBackRatioActions->actions()[0]->setChecked(true);
    else if (mode == Qt::KeepAspectRatio)
        m_gBackRatioActions->actions()[1]->setChecked(true);
    else if (mode == Qt::IgnoreAspectRatio)
        m_gBackRatioActions->actions()[2]->setChecked(true);
}

void MainWindow::slotShowPropertiesWidget(QWidget * widget)
{
    // delete current Properties content
    QLayoutItem * prevItem = ui->propLayout->takeAt(0);
    if (prevItem) {
        delete prevItem->widget();
        delete prevItem;
    }

    // show the Properties container with new content and title
    if (widget) {
        ui->widgetCanvas->collapse();
        widget->setParent(ui->widgetProperties);
        ui->propLayout->addWidget(widget);
        ui->widgetProperties->setTitle(widget->windowTitle());
        ui->widgetProperties->expand();
    }
    // or show the Canvas containter
    else {
        ui->widgetProperties->collapse();
        ui->widgetCanvas->expand();
    }
}

void MainWindow::slotVerifyTutorial(QNetworkReply * reply)
{
    if (reply->error() != QNetworkReply::NoError)
        return;

    QString htmlCode = reply->readAll();
    bool tutorialValid = htmlCode.contains(TUTORIAL_STRING, Qt::CaseInsensitive);
    m_aHelpTutorial->setVisible(tutorialValid);
}

void MainWindow::slotVerifySupport(/*const KnowledgeItemV1List & items*/)
{
    int supportEntries = 0;
    m_aHelpSupport->setVisible(supportEntries > 0);
    m_aHelpSupport->setText(tr("Support (%1)").arg(supportEntries));
/*
    qWarning("MainWindow::slotOcsKbItems: got %d items", items.size());
    foreach (KnowledgeItemV1 * item, items) {
        qWarning() << item->name() << item->description() << item->answer();
    }
*/
}

void MainWindow::slotVerifyVideoInputs(int count)
{
    // maybe blink or something?
    ui->aAddWebcam->setVisible(count > 0);
}