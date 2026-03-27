#include "mainwindow.h"
#include "settingsdialog.h"
#include "aboutdialog.h"
#include "errorlogger.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QKeyEvent>
#include <QScreen>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QSizePolicy>
#include <QFont>
#include <QPalette>
#include <QSplitter>

// ─── Constructor ────────────────────────────────────────────────────────────
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_appPath = QCoreApplication::applicationDirPath();

    // Settings
    m_set = new SettingsManager(m_appPath + "/jp.ini");
    if (!QFileInfo::exists(m_appPath + "/jp.ini")) {
        m_set->applyDefaults(m_appPath);
    }
    m_set->load();

    // Error logger
    ErrorLogger::instance().setEnabled(m_set->errorLog);
    ErrorLogger::instance().setFilePath(m_appPath + "/Error.log");
    ErrorLogger::instance().log("", "***********************************************************");
    ErrorLogger::instance().log("", "Starting program...");

    // Palette manager
    m_pal = new PaletteManager(m_appPath + "/palette.ini");

    // Language manager
    m_lang = new LanguageManager(m_appPath + "/language.ini");
    m_lang->setLanguage(m_set->language);

    // Audio engine
    m_audio = new AudioEngine(this);
    m_audio->init(m_set->device);
    m_audio->setMasterVolume(m_set->volume);
    connect(m_audio, &AudioEngine::jingleFinished,     this, &MainWindow::onJingleFinished);
    connect(m_audio, &AudioEngine::vuLevelsUpdated,    this, &MainWindow::onVuLevels);
    connect(m_audio, &AudioEngine::streamStatusChanged,this, &MainWindow::onStreamStatusChanged);

    // Build UI
    setupUi();
    applySettings();
    restoreWindowState();

    // Load last palette
    refreshPaletteList();
    if (!m_set->paletteIndex.isEmpty())
        loadPalette(m_set->paletteIndex);

    // Load URL history
    QString urlFile = m_appPath + "/urls.txt";
    if (QFileInfo::exists(urlFile)) {
        QFile f(urlFile);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&f);
            while (!in.atEnd())
                m_cmbStream->addItem(in.readLine().trimmed());
        }
    }
    if (m_cmbStream->count() > 0)
        m_cmbStream->setCurrentIndex(m_cmbStream->count() - 1);

    // Load time announce
    m_audio->loadTimeAnnounce(m_set->timeAnnouncer, "");

    // Timers
    m_timer1 = new QTimer(this); m_timer1->setInterval(500);
    m_timer2 = new QTimer(this); m_timer2->setInterval(1000);
    m_timer3 = new QTimer(this); m_timer3->setInterval(80);
    m_timer5 = new QTimer(this); m_timer5->setInterval(200);

    connect(m_timer1, &QTimer::timeout, this, &MainWindow::onTimer1);
    connect(m_timer2, &QTimer::timeout, this, &MainWindow::onTimer2);
    connect(m_timer3, &QTimer::timeout, this, &MainWindow::onTimer3);
    connect(m_timer5, &QTimer::timeout, this, &MainWindow::onTimer5);

    m_timer1->start(); m_timer2->start();
    m_timer3->start(); m_timer5->start();

    setWindowTitle("Jingle Palette Pro v0.1");
    updateDisplayTime();
    updatePaletteDisplay();

    // --- Estilo global escuro inspirado no exemplo PyQt ---
    QString darkStyle = R"(
        QWidget { background-color: #222; color: #eee; }
        QMenuBar { background-color: #222; color: #eee; }
        QMenuBar::item { background-color: transparent; padding: 4px 10px; }
        QMenuBar::item:selected { background-color: #2e5f2e; color: #f0fff0; }
        QMenu { background-color: #222; color: #eee; border: 1px solid #333; }
        QMenu::item { padding: 6px 18px; background-color: transparent; }
        QMenu::item:selected { background-color: #2e5f2e; color: #f0fff0; }
        QTableWidget { background-color: #222; color: #eee; gridline-color: #333; selection-background-color: transparent; selection-color: #eee; font-size: 14px; }
        QTableWidget::item:selected { background-color: transparent; border: 1px solid #7CFC00; }
        QHeaderView::section { background-color: #2a2a2a; color: #ddd; border: 1px solid #333; font-size: 14px; font-weight: 600; }
        QLabel { color: #eee; }
        QCheckBox { color: #eee; }
        QPushButton { background-color: #444; color: #eee; border: 2px solid #222; }
    )";
    qApp->setStyleSheet(darkStyle);
}

MainWindow::~MainWindow()
{
    m_audio->free();
}

// ─── Setup UI ───────────────────────────────────────────────────────────────
void MainWindow::setupUi()
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QHBoxLayout *hMain = new QHBoxLayout(central);
    hMain->setSpacing(4);
    hMain->setContentsMargins(4, 4, 4, 4);

    // Esquerda: display, grid, tabs
    QVBoxLayout *vLeft = new QVBoxLayout;
    vLeft->setSpacing(4);
    vLeft->setContentsMargins(0, 0, 0, 0);

    // Display
    setupDisplay();
    vLeft->addWidget(m_displayFrame);

    // Grid
    setupJingleGrid();
    QWidget *gridWidget = new QWidget;
    QGridLayout *grid = new QGridLayout(gridWidget);
    grid->setSpacing(3);
    for (int i = 0; i < JINGLE_COUNT; ++i) {
        grid->addWidget(m_jingBtns[i], i / 5, i % 5);
    }
    vLeft->addWidget(gridWidget, 1);

    // Tabs
    setupTabPanel();
    vLeft->addWidget(m_tabWidget);

    hMain->addLayout(vLeft, 1);

    // Direita: VU meter estéreo vertical lado a lado
    setupVuMeter();
    QHBoxLayout *hVu = new QHBoxLayout;
    hVu->setSpacing(8);
    hVu->setContentsMargins(0, 0, 0, 0);
    m_vuL->setMinimumWidth(32);
    m_vuR->setMinimumWidth(32);
    m_vuL->setMinimumHeight(200);
    m_vuR->setMinimumHeight(200);
    hVu->addWidget(m_vuL, 1);
    hVu->addWidget(m_vuR, 1);
    hMain->addLayout(hVu);

    setupContextMenu();
}

void MainWindow::setupDisplay()
{
    m_displayFrame = new QFrame;
    m_displayFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    m_displayFrame->setStyleSheet("background-color: #1a1a1a;");
    m_displayFrame->setMinimumHeight(110);

    QVBoxLayout *vDisp = new QVBoxLayout(m_displayFrame);
    vDisp->setSpacing(2);
    vDisp->setContentsMargins(6, 4, 6, 4);

    // Row 1: remaining time | icons | current time
    QHBoxLayout *row1 = new QHBoxLayout;

    m_lblRemTime = new QLabel("-00.00");
    m_lblRemTime->setStyleSheet("color: #FF4444; font-size: 22px; font-weight: bold; font-family: 'Courier New';");

    m_lblWarnIcon = new QLabel("⚠");
    m_lblWarnIcon->setStyleSheet("color: #FF8800; font-size: 14px;");
    m_lblWarnIcon->setVisible(false);

    m_lblTaIcon = new QLabel("🕐");
    m_lblTaIcon->setStyleSheet("color: #00AAFF; font-size: 14px;");
    m_lblTaIcon->setVisible(false);

    m_lblErrIcon = new QLabel("✗");
    m_lblErrIcon->setStyleSheet("color: #FF4444; font-size: 16px; font-weight: bold;");
    m_lblErrIcon->setVisible(false);

    m_lblTime = new QLabel("00:00:00");
    m_lblTime->setStyleSheet("color: #1aff1a; font-size: 22px; font-weight: bold; font-family: 'Courier New'; background: transparent;");
    m_lblTime->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    row1->addWidget(m_lblRemTime);
    row1->addWidget(m_lblWarnIcon);
    row1->addWidget(m_lblTaIcon);
    row1->addWidget(m_lblErrIcon);
    row1->addStretch();
    row1->addWidget(m_lblTime);

    // Row 2: palette navigation + date/day/week
    QHBoxLayout *row2 = new QHBoxLayout;

    m_btnPrev = new QPushButton("◄");
    m_btnPrev->setFixedSize(20, 20);
    m_btnPrev->setStyleSheet("QPushButton{color:#aaa;background:#333;border:none;font-size:9px;}"
                             "QPushButton:hover{background:#555;}");
    connect(m_btnPrev, &QPushButton::clicked, this, &MainWindow::onPrevPalette);

    m_lblPrevPal = new QLabel("");
    m_lblPrevPal->setStyleSheet("color: #888888; font-size: 9px;");

    m_lblCurPal = new QLabel("Demo");
    m_lblCurPal->setStyleSheet("color: #FFFFFF; font-size: 11px; font-weight: bold;");

    m_lblNextPal = new QLabel("");
    m_lblNextPal->setStyleSheet("color: #888888; font-size: 9px;");

    m_btnNext = new QPushButton("►");
    m_btnNext->setFixedSize(20, 20);
    m_btnNext->setStyleSheet("QPushButton{color:#aaa;background:#333;border:none;font-size:9px;}"
                             "QPushButton:hover{background:#555;}");
    connect(m_btnNext, &QPushButton::clicked, this, &MainWindow::onNextPalette);

    // Week number
    m_lblWeek = new QLabel("1");
    m_lblWeek->setStyleSheet("color: #FFFFC0; font-size: 11px;");
    m_lblWeek->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    row2->addWidget(m_btnPrev);
    row2->addWidget(m_lblPrevPal);
    row2->addSpacing(4);
    row2->addWidget(m_lblCurPal);
    row2->addSpacing(4);
    row2->addWidget(m_lblNextPal);
    row2->addWidget(m_btnNext);
    row2->addStretch();
    row2->addWidget(m_lblWeek);

    // Row 3: só hora e data à direita
    QVBoxLayout *vDateTime = new QVBoxLayout;
    vDateTime->setSpacing(0);
    vDateTime->setContentsMargins(0,0,0,0);
    m_lblTime->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    vDateTime->addWidget(m_lblTime, 0, Qt::AlignRight);
    m_lblDate = new QLabel("2026/03/27");
    m_lblDate->setStyleSheet("color: #1aff1a; font-size: 18px; font-weight: bold; background: transparent;");
    m_lblDate->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    vDateTime->addWidget(m_lblDate, 0, Qt::AlignRight);

    // Remover qualquer label extra (ex: m_lblWeek) do layout
    if (m_lblWeek) {
        m_lblWeek->hide();
    }
    QHBoxLayout *row3 = new QHBoxLayout;
    row3->addStretch();
    row3->addLayout(vDateTime);
    vDisp->addLayout(row1);
    vDisp->addLayout(row2);
    vDisp->addLayout(row3);
}

void MainWindow::setupJingleGrid()
{
    for (int i = 0; i < JINGLE_COUNT; ++i) {
        m_jingBtns[i] = new JingleButton(i, this);
        m_jingBtns[i]->setText(QString::number(i + 1));
        connect(m_jingBtns[i], &QPushButton::clicked,
                this, [this, i]() { onJingleClicked(i); });
        connect(m_jingBtns[i], &JingleButton::rightClicked,
                this, &MainWindow::onJingleRightClicked);
    }
}

void MainWindow::setupVuMeter()
{
    m_vuL = new VuMeter(Qt::Vertical);
    m_vuR = new VuMeter(Qt::Vertical);
    m_vuL->setObjectName("VuL");
    m_vuR->setObjectName("VuR");
    m_vuL->setFixedWidth(14);
    m_vuR->setFixedWidth(14);
    m_vuL->setMinimumHeight(320);
    m_vuR->setMinimumHeight(320);
    // Adicionar margem inferior para afastar do texto L/R
    m_vuL->setContentsMargins(0, 0, 0, 16);
    m_vuR->setContentsMargins(0, 0, 0, 16);
}

void MainWindow::setupTabPanel()
{
    m_tabWidget = new QTabWidget;
    m_tabWidget->setMaximumHeight(200);

    // ── Tab 0: Palettes ──────────────────────────────────────────────────
    {
        QWidget *w = new QWidget;
        QHBoxLayout *h = new QHBoxLayout(w);

        QVBoxLayout *vLeft = new QVBoxLayout;
        m_ckAssign = new QCheckBox(tr("Assign Jingle To Button"));
        m_btSave   = new QPushButton(tr("Save Current Palette"));
        m_btNew    = new QPushButton(tr("Stop and Empty Palette"));
        m_txtSave  = new QLineEdit;
        m_txtSave->setPlaceholderText(tr("Palette name, press Enter"));
        m_txtSave->setVisible(false);

        vLeft->addWidget(m_ckAssign);
        vLeft->addWidget(m_btSave);
        vLeft->addWidget(m_txtSave);
        vLeft->addWidget(m_btNew);
        vLeft->addStretch();

        m_lstPal = new QListWidget;
        m_lstPal->setSortingEnabled(true);

        h->addLayout(vLeft);
        h->addWidget(m_lstPal, 1);

        connect(m_ckAssign, &QCheckBox::toggled,   this, &MainWindow::onAssignToggled);
        connect(m_btSave,   &QPushButton::clicked, this, &MainWindow::onSavePalette);
        connect(m_btNew,    &QPushButton::clicked, this, &MainWindow::onNewPalette);
        connect(m_txtSave,  &QLineEdit::returnPressed, this, [this]() {
            QString name = m_txtSave->text().trimmed();
            if (!name.isEmpty()) {
                m_pal->savePalette(name, m_audio->jings);
                refreshPaletteList();
                m_lstPal->setCurrentRow(m_lstPal->findItems(name, Qt::MatchExactly).isEmpty() ? 0 :
                    m_lstPal->row(m_lstPal->findItems(name, Qt::MatchExactly).first()));
                m_set->paletteIndex = name;
            }
            m_txtSave->setVisible(false);
        });
        connect(m_lstPal, &QListWidget::currentTextChanged, this, &MainWindow::onPaletteSelected);

        m_tabWidget->addTab(w, tr("Palettes"));
    }

    // ── Tab 1: Time Announce ────────────────────────────────────────────
    {
        QWidget *w = new QWidget;
        QGridLayout *g = new QGridLayout(w);

        m_ckTmAnn   = new QCheckBox(tr("Time Announce"));
        m_btTmAnJin = new QCheckBox(tr("Select jingle to be announced..."));
        m_btTmAnJin->setEnabled(false);

        m_spMin = new QSpinBox; m_spMin->setRange(0,59); m_spMin->setValue(m_set->timeAnnMin);
        m_spSec = new QSpinBox; m_spSec->setRange(0,59); m_spSec->setValue(m_set->timeAnnSec);
        m_spDel = new QSpinBox; m_spDel->setRange(0,59); m_spDel->setValue(m_set->timeAnnDel);

        g->addWidget(new QLabel(tr("Minute:")), 0, 0);
        g->addWidget(m_spMin, 0, 1);
        g->addWidget(new QLabel(tr("Second:")), 1, 0);
        g->addWidget(m_spSec, 1, 1);
        g->addWidget(new QLabel(tr("Jingle delay (s):")), 2, 0);
        g->addWidget(m_spDel, 2, 1);
        g->addWidget(m_ckTmAnn,   0, 2, 1, 2);
        g->addWidget(m_btTmAnJin, 1, 2, 1, 2);
        g->setColumnStretch(3, 1);

        connect(m_ckTmAnn,   &QCheckBox::toggled, this, &MainWindow::onTimeAnnounceToggled);
        connect(m_btTmAnJin, &QCheckBox::toggled, this, &MainWindow::onSelectTaJingle);
        connect(m_spMin, QOverload<int>::of(&QSpinBox::valueChanged), this,
                [this](int v){ m_set->timeAnnMin = v; });
        connect(m_spSec, QOverload<int>::of(&QSpinBox::valueChanged), this,
                [this](int v){ m_set->timeAnnSec = v; });
        connect(m_spDel, QOverload<int>::of(&QSpinBox::valueChanged), this,
                [this](int v){ m_set->timeAnnDel = v; });

        m_tabWidget->addTab(w, tr("Time Announce"));
    }

    // ── Tab 2: Play Stream ──────────────────────────────────────────────
    {
        QWidget *w = new QWidget;
        QVBoxLayout *v = new QVBoxLayout(w);

        m_cmbStream  = new QComboBox;
        m_cmbStream->setEditable(true);
        m_cmbStream->setInsertPolicy(QComboBox::NoInsert);
        m_cmbStream->lineEdit()->setPlaceholderText("http://");
        m_cmbStream->setToolTip(tr("Enter the location. Eg. http://yourserver.com/yourfile.mp3"));

        m_ckStream   = new QCheckBox(tr("Play Remote Location"));
        m_ckMixByTa  = new QCheckBox(tr("Auto Mix by Time Announce"));

        m_lblConDisp = new QLabel(tr("Not connected"));
        m_lblConDisp->setAlignment(Qt::AlignCenter);
        m_lblConDisp->setStyleSheet("background:#000; color:#FFFFC0; padding:2px;");

        v->addWidget(new QLabel(tr("Remote location URL:")));
        v->addWidget(m_cmbStream);
        QHBoxLayout *h = new QHBoxLayout;
        h->addWidget(m_ckStream);
        h->addWidget(m_ckMixByTa);
        v->addLayout(h);
        v->addWidget(m_lblConDisp);

        connect(m_ckStream, &QCheckBox::toggled, this, &MainWindow::onPlayStreamToggled);

        m_tabWidget->addTab(w, tr("Play Stream"));
    }

    // ── Tab 3: Settings ─────────────────────────────────────────────────
    {
        QWidget *w = new QWidget;
        QHBoxLayout *h = new QHBoxLayout(w);

        // Volume slider (vertical)
        QVBoxLayout *vVol = new QVBoxLayout;
        m_lblVol    = new QLabel("0 dB");
        m_lblVolExp = new QLabel(tr("General Volume"));
        m_lblVolExp->setStyleSheet("font-size:9px;");
        m_slideVol  = new QSlider(Qt::Horizontal);
        m_slideVol->setRange(0, 100);
        m_slideVol->setValue(m_set->volume);
        m_slideVol->setToolTip(tr("General Volume"));
        connect(m_slideVol, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);

        vVol->addWidget(m_lblVolExp);
        vVol->addWidget(m_slideVol);
        vVol->addWidget(m_lblVol);

        // Checkboxes
        QVBoxLayout *vCk = new QVBoxLayout;
        m_ckAssVol  = new QCheckBox(tr("Assign new Volume level to Jingle"));
        m_ckLoopI   = new QCheckBox(tr("Assign/Clear Loop Mode"));
        m_ckTouch   = new QCheckBox(tr("Touch Play Mode (H)"));
        m_ckAutoRep = new QCheckBox(tr("Auto Repeat (R)"));
        m_ckAutoMix = new QCheckBox(tr("Auto Mix (M)"));
        vCk->addWidget(m_ckAssVol);
        vCk->addWidget(m_ckLoopI);
        vCk->addWidget(m_ckTouch);
        vCk->addWidget(m_ckAutoRep);
        vCk->addWidget(m_ckAutoMix);

        // Buttons
        QVBoxLayout *vBt = new QVBoxLayout;
        m_btSettings = new QPushButton(tr("Settings..."));
        m_btAbout    = new QPushButton(tr("About..."));
        connect(m_btSettings, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);
        connect(m_btAbout,    &QPushButton::clicked, this, &MainWindow::onAboutClicked);
        vBt->addWidget(m_btSettings);
        vBt->addWidget(m_btAbout);
        vBt->addStretch();

        h->addLayout(vVol, 2);
        h->addLayout(vCk, 2);
        h->addLayout(vBt, 1);

        m_tabWidget->addTab(w, tr("Settings"));
    }
}

void MainWindow::setupContextMenu()
{
    m_ctxMenu  = new QMenu(this);
    m_actPlay  = m_ctxMenu->addAction(tr("Play from start"));
    m_actPause = m_ctxMenu->addAction(tr("Pause"));
    m_actStop  = m_ctxMenu->addAction(tr("Stop"));
    m_ctxMenu->addSeparator();
    m_actAssign  = m_ctxMenu->addAction(tr("Assign..."));
    m_actClear   = m_ctxMenu->addAction(tr("Clear"));
    m_ctxMenu->addSeparator();
    m_actLoop    = m_ctxMenu->addAction(tr("Toggle Loop"));
    m_actVolume  = m_ctxMenu->addAction(tr("Set Volume..."));
    m_actTa      = m_ctxMenu->addAction(tr("Use as Time Announce Jingle"));

    connect(m_actPlay,   &QAction::triggered, this, &MainWindow::onMenuPlay);
    connect(m_actPause,  &QAction::triggered, this, &MainWindow::onMenuPause);
    connect(m_actStop,   &QAction::triggered, this, &MainWindow::onMenuStop);
    connect(m_actAssign, &QAction::triggered, this, &MainWindow::onMenuAssign);
    connect(m_actClear,  &QAction::triggered, this, &MainWindow::onMenuClear);
    connect(m_actLoop,   &QAction::triggered, this, &MainWindow::onMenuLoop);
    connect(m_actVolume, &QAction::triggered, this, &MainWindow::onMenuVolume);
    connect(m_actTa,     &QAction::triggered, this, &MainWindow::onMenuTimeAnnounce);
}

// ─── Apply / save settings ───────────────────────────────────────────────────
void MainWindow::applySettings()
{
    setWindowFlag(Qt::WindowStaysOnTopHint, m_set->alwaysOnTop);
    show();

    m_ckTouch->setChecked(m_set->touch);
    m_ckAutoRep->setChecked(m_set->autoRepeat);
    m_ckAutoMix->setChecked(m_set->autoMix);
    m_tabWidget->setCurrentIndex(m_set->activeTab);
    m_slideVol->setValue(m_set->volume);
    m_spMin->setValue(m_set->timeAnnMin);
    m_spSec->setValue(m_set->timeAnnSec);
    m_spDel->setValue(m_set->timeAnnDel);
}

void MainWindow::restoreWindowState()
{
    QScreen *scr = QGuiApplication::primaryScreen();
    QRect avail  = scr->availableGeometry();

    int w = m_set->winWidth  > 100 ? m_set->winWidth  : 1000;
    int h = m_set->winHeight > 100 ? m_set->winHeight : 700;
    resize(w, h);

    int x = m_set->posLeft, y = m_set->posTop;
    if (x < 0 || y < 0 || x + w > avail.right() || y + h > avail.bottom()) {
        x = avail.center().x() - w / 2;
        y = avail.center().y() - h / 2;
    }
    move(x, y);

    if (m_set->winState == 2) showMaximized();
}

void MainWindow::saveWindowState()
{
    if (!isMaximized()) {
        m_set->winWidth  = width();
        m_set->winHeight = height();
        m_set->posLeft   = x();
        m_set->posTop    = y();
    }
    m_set->winState   = isMaximized() ? 2 : 0;
    m_set->activeTab  = m_tabWidget->currentIndex();
    m_set->touch      = m_ckTouch->isChecked();
    m_set->autoRepeat = m_ckAutoRep->isChecked();
    m_set->autoMix    = m_ckAutoMix->isChecked();
    m_set->volume     = m_slideVol->value();
    m_set->timeAnnMin = m_spMin->value();
    m_set->timeAnnSec = m_spSec->value();
    m_set->timeAnnDel = m_spDel->value();

    if (m_lstPal->currentItem())
        m_set->paletteIndex = m_lstPal->currentItem()->text();

    m_set->save();
}

// ─── Close / key events ──────────────────────────────────────────────────────
void MainWindow::closeEvent(QCloseEvent *e)
{
    saveWindowState();
    m_audio->free();
    ErrorLogger::instance().log("", "Program normal exit");
    ErrorLogger::instance().log("", "***********************************************************");
    e->accept();
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    QMainWindow::resizeEvent(e);
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_R:
        m_ckAutoRep->setChecked(!m_ckAutoRep->isChecked());
        break;
    case Qt::Key_H:
        m_ckTouch->setChecked(!m_ckTouch->isChecked());
        break;
    case Qt::Key_M:
        m_ckAutoMix->setChecked(!m_ckAutoMix->isChecked());
        break;
    case Qt::Key_E:
        m_tabWidget->setCurrentIndex(0);
        onNewPalette();
        break;
    case Qt::Key_N:
        m_tabWidget->setCurrentIndex(1);
        m_ckTmAnn->setChecked(!m_ckTmAnn->isChecked());
        break;
    case Qt::Key_A:
        m_tabWidget->setCurrentIndex(0);
        m_ckAssign->setChecked(!m_ckAssign->isChecked());
        break;
    case Qt::Key_S:
        m_tabWidget->setCurrentIndex(0);
        onSavePalette();
        break;
    case Qt::Key_L:
        m_tabWidget->setCurrentIndex(3);
        break;
    case Qt::Key_Delete:
        // Clear selected button (no context menu needed)
        break;
    default:
        QMainWindow::keyPressEvent(e);
    }
}

// ─── Jingle operations ───────────────────────────────────────────────────────
void MainWindow::onJingleClicked(int idx)
{
    if (m_ckAssign->isChecked()) {
        assignJingle(idx);
        return;
    }
    if (m_btTmAnJin->isChecked() && !m_audio->jings[idx].path.isEmpty()) {
        // Select this jingle as time announce jingle
        m_audio->loadTimeAnnounce(m_set->timeAnnouncer, m_audio->jings[idx].path);
        m_btTmAnJin->setChecked(false);
        m_btTmAnJin->setStyleSheet("background-color: #FFC0C0;");
        return;
    }
    if (m_ckAssVol->isChecked() && !m_audio->jings[idx].path.isEmpty()) {
        m_volSelIdx  = idx;
        m_ckAssVol->setVisible(false);
        m_lblVolExp->setText(m_jingBtns[idx]->text() + ":");
        m_slideVol->setValue(m_audio->jings[idx].volume);
        return;
    }
    if (m_ckLoopI->isChecked() && !m_audio->jings[idx].path.isEmpty()) {
        bool newLoop = !m_audio->jings[idx].loop;
        m_audio->setJingleLoop(idx, newLoop);
        m_jingBtns[idx]->setLoopIndicatorVisible(newLoop);
        m_ckLoopI->setChecked(false);
        return;
    }
    playJingle(idx);
}

void MainWindow::assignJingle(int idx)
{
    if (m_audio->isPlaying(idx)) return;
    m_ckAssign->setChecked(false);

    QString path = QFileDialog::getOpenFileName(this,
        m_lang->entry("mDlgAsBu", tr("Assign Jingle to Button")), "",
        tr("Audio Files (*.wav *.mp3 *.mp2 *.mp1 *.mpa *.ogg)"));

    if (path.isEmpty()) return;

    m_audio->jings[idx].path = path;
    m_audio->jings[idx].loop = false;
    m_audio->loadJingle(idx, path);
    m_jingBtns[idx]->setText(formatJingleName(path));
    m_jingBtns[idx]->setLoopIndicatorVisible(false);
}

void MainWindow::playJingle(int idx)
{
    if (m_audio->jings[idx].path.isEmpty()) {
        // Empty slot — ignore
        return;
    }
    if (m_ckTouch->isChecked()) {
        // Touch mode: play immediately, keep playing on release
        m_audio->playJingle(idx);
        m_jingBtns[idx]->setPlaying(true);
        m_vuTrackIdx = idx;
        return;
    }

    // Se outro jingle está tocando, marcar como aguardando parar
    for (int i = 0; i < JINGLE_COUNT; ++i) {
        if (i != idx && m_audio->isPlaying(i)) {
            m_jingBtns[i]->setWaitingToStop(true);
        }
    }
    if (m_audio->isPlaying(idx)) {
        if (m_ckAutoRep->isChecked()) {
            m_audio->seekJingle(idx, 0);
        } else {
            m_audio->stopJingle(idx);
            m_jingBtns[idx]->setPlaying(false);
        }
    } else {
        m_audio->playJingle(idx);
        m_jingBtns[idx]->setPlaying(true);
        m_vuTrackIdx = idx;

        // Auto-mix: fade out other jingles
        if (m_ckAutoMix->isChecked()) {
            m_audio->startAutoMix(idx, m_set->autoMixTime);
        }
    }
}

void MainWindow::stopAll()
{
    for (int i = 0; i < JINGLE_COUNT; ++i) {
        m_audio->stopJingle(i);
        m_jingBtns[i]->setPlaying(false);
    }
    m_vuTrackIdx = -1;
}

void MainWindow::onJingleFinished(int idx)
{
    m_jingBtns[idx]->setPlaying(false);
    m_jingBtns[idx]->setPaused(false);
    m_jingBtns[idx]->setWaitingToStop(false);
    if (m_vuTrackIdx == idx) m_vuTrackIdx = -1;
}

// ─── Context menu ────────────────────────────────────────────────────────────
void MainWindow::onJingleRightClicked(int idx)
{
    m_ctxIdx = idx;
    bool hasFile = !m_audio->jings[idx].path.isEmpty();
    m_actPlay->setEnabled(hasFile);
    m_actPause->setEnabled(hasFile && m_audio->isPlaying(idx));
    m_actStop->setEnabled(hasFile && (m_audio->isPlaying(idx) || m_audio->isPaused(idx)));
    m_actClear->setEnabled(hasFile);
    m_actLoop->setEnabled(hasFile && !m_audio->isPlaying(idx));
    m_actVolume->setEnabled(hasFile);
    m_actTa->setEnabled(hasFile);
    m_actPause->setText(m_audio->isPaused(idx) ? tr("Resume") : tr("Pause"));
    m_ctxMenu->exec(QCursor::pos());
}

void MainWindow::onMenuPlay()
{
    if (m_ctxIdx < 0) return;
    if (m_audio->isPaused(m_ctxIdx)) {
        m_audio->seekJingle(m_ctxIdx, 0);
    }
    m_audio->playJingle(m_ctxIdx);
    m_jingBtns[m_ctxIdx]->setPlaying(true);
}

void MainWindow::onMenuPause()
{
    if (m_ctxIdx < 0) return;
    if (m_audio->isPaused(m_ctxIdx)) {
        m_audio->resumeJingle(m_ctxIdx);
        m_jingBtns[m_ctxIdx]->setPlaying(true);
    } else {
        m_audio->pauseJingle(m_ctxIdx);
        m_jingBtns[m_ctxIdx]->setPaused(true);
    }
}

void MainWindow::onMenuStop()
{
    if (m_ctxIdx < 0) return;
    m_audio->stopJingle(m_ctxIdx);
    m_jingBtns[m_ctxIdx]->setPlaying(false);
}

void MainWindow::onMenuAssign()
{
    if (m_ctxIdx < 0) return;
    assignJingle(m_ctxIdx);
}

void MainWindow::onMenuClear()
{
    if (m_ctxIdx < 0) return;
    if (m_audio->jings[m_ctxIdx].path.isEmpty()) return;
    if (QMessageBox::question(this, "Jingle Palette Pro v0.1",
            m_lang->entry("mMsdb", tr("Clear this jingle button?")),
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        m_audio->stopJingle(m_ctxIdx);
        m_audio->jings[m_ctxIdx].path   = "";
        m_audio->jings[m_ctxIdx].loop   = false;
        m_audio->jings[m_ctxIdx].volume = 100;
        m_audio->loadJingle(m_ctxIdx, "");
        m_jingBtns[m_ctxIdx]->setText(QString::number(m_ctxIdx + 1));
        m_jingBtns[m_ctxIdx]->setPlaying(false);
        m_jingBtns[m_ctxIdx]->setLoopIndicatorVisible(false);
    }
}

void MainWindow::onMenuLoop()
{
    if (m_ctxIdx < 0) return;
    bool nl = !m_audio->jings[m_ctxIdx].loop;
    m_audio->setJingleLoop(m_ctxIdx, nl);
    m_jingBtns[m_ctxIdx]->setLoopIndicatorVisible(nl);
}

void MainWindow::onMenuVolume()
{
    if (m_ctxIdx < 0) return;
    m_tabWidget->setCurrentIndex(3);
    m_volSelIdx = m_ctxIdx;
    m_ckAssVol->setVisible(false);
    m_lblVolExp->setText(m_jingBtns[m_ctxIdx]->text() + ":");
    m_slideVol->setValue(m_audio->jings[m_ctxIdx].volume);
}

void MainWindow::onMenuTimeAnnounce()
{
    if (m_ctxIdx < 0) return;
    m_audio->loadTimeAnnounce(m_set->timeAnnouncer,
                               m_audio->jings[m_ctxIdx].path);
    m_btTmAnJin->setChecked(true);
}

// ─── Palette operations ───────────────────────────────────────────────────────
void MainWindow::refreshPaletteList()
{
    m_lstPal->blockSignals(true);
    m_lstPal->clear();
    for (const QString &name : m_pal->paletteNames())
        m_lstPal->addItem(name);
    m_lstPal->sortItems();
    m_lstPal->blockSignals(false);

    updatePaletteDisplay();
}

void MainWindow::loadPalette(const QString &name)
{
    // Stop any playing jingles first
    for (int i = 0; i < JINGLE_COUNT; ++i) {
        if (!m_audio->isPlaying(i)) {
            m_audio->stopJingle(i);
        }
    }

    m_pal->loadPalette(name, m_audio->jings);

    // Reload streams and update buttons
    for (int i = 0; i < JINGLE_COUNT; ++i) {
        if (!m_audio->isPlaying(i)) {
            m_audio->loadJingle(i, m_audio->jings[i].path);
            m_jingBtns[i]->setText(m_audio->jings[i].path.isEmpty() ?
                QString::number(i + 1) : formatJingleName(m_audio->jings[i].path));
            m_jingBtns[i]->setPlaying(false);
            m_jingBtns[i]->setLoopIndicatorVisible(m_audio->jings[i].loop);
            m_audio->setJingleLoop(i, m_audio->jings[i].loop);
        }
    }

    m_set->paletteIndex = name;
    updatePaletteDisplay();
}

void MainWindow::onPaletteSelected(const QString &name)
{
    if (name.isEmpty()) return;
    loadPalette(name);
    // Select in list
    auto items = m_lstPal->findItems(name, Qt::MatchExactly);
    if (!items.isEmpty())
        m_lstPal->setCurrentItem(items.first());
}

void MainWindow::onAssignToggled(bool on)
{
    if (on) {
        m_ckAssign->setStyleSheet("background-color: #FF80FF;");
    } else {
        m_ckAssign->setStyleSheet("");
    }
}

void MainWindow::onSavePalette()
{
    m_ckAssign->setChecked(false);
    m_txtSave->setVisible(true);
    m_txtSave->setFocus();
    m_txtSave->clear();
}

void MainWindow::onNewPalette()
{
    stopAll();
    for (int i = 0; i < JINGLE_COUNT; ++i) {
        m_audio->jings[i].path   = "";
        m_audio->jings[i].loop   = false;
        m_audio->jings[i].volume = 100;
        m_audio->loadJingle(i, "");
        m_jingBtns[i]->setText(QString::number(i + 1));
        m_jingBtns[i]->setLoopIndicatorVisible(false);
        m_jingBtns[i]->setPlaying(false);
    }
    updatePaletteDisplay();
}

void MainWindow::onPrevPalette()
{
    int row = m_lstPal->currentRow();
    if (row > 0) {
        m_lstPal->setCurrentRow(row - 1);
        onPaletteSelected(m_lstPal->currentItem()->text());
    }
}

void MainWindow::onNextPalette()
{
    int row = m_lstPal->currentRow();
    if (row < m_lstPal->count() - 1) {
        m_lstPal->setCurrentRow(row + 1);
        onPaletteSelected(m_lstPal->currentItem()->text());
    }
}

// ─── Stream ───────────────────────────────────────────────────────────────────
void MainWindow::onPlayStreamToggled(bool on)
{
    if (on) {
        QString url = m_cmbStream->currentText().trimmed();
        if (url.isEmpty() || url == "http://") {
            m_ckStream->setChecked(false);
            return;
        }
        m_lblConDisp->setText(m_lang->entry("mStrPla", tr("Connecting...")));
        m_audio->playStream(QUrl(url));

        // Save URL
        if (m_cmbStream->findText(url) < 0)
            m_cmbStream->addItem(url);
        QFile f(m_appPath + "/urls.txt");
        if (f.open(QIODevice::Append | QIODevice::Text))
            QTextStream(&f) << url << "\n";
    } else {
        m_audio->stopStream();
        m_lblConDisp->setText(m_lang->entry("mStrNoCo", tr("Not connected")));
    }
}

void MainWindow::onStreamStatusChanged(const QString &status)
{
    m_lblConDisp->setText(status);
    if (!m_audio->isStreamPlaying() && m_ckStream->isChecked()) {
        m_ckStream->setChecked(false);
    }
}

// ─── Time Announce ────────────────────────────────────────────────────────────
void MainWindow::onTimeAnnounceToggled(bool on)
{
    m_btTmAnJin->setEnabled(on);
    m_lblTaIcon->setVisible(on);
    if (!on) m_audio->stopTimeAnnounce();
}

void MainWindow::onSelectTaJingle(bool on)
{
    if (on) {
        m_btTmAnJin->setStyleSheet("background-color: #FFC0C0;");
    } else {
        m_btTmAnJin->setStyleSheet("");
    }
}

// ─── Volume ───────────────────────────────────────────────────────────────────
void MainWindow::onVolumeChanged(int val)
{
    if (m_volSelIdx >= 0 && !m_ckAssVol->isVisible()) {
        // Assigning individual jingle volume
        m_audio->setJingleVolume(m_volSelIdx, val);
        m_lblVol->setText(QString::number(val - 100) + " dB");
    } else {
        // Master volume
        m_audio->setMasterVolume(val);
        m_set->volume = val;
        m_lblVol->setText(QString::number(val - 100) + " dB");
    }
}

// ─── Timers ───────────────────────────────────────────────────────────────────
void MainWindow::onTimer1()
{
    // Flash assign checkbox
    if (m_ckAssign->isChecked()) {
        m_flashCount++;
        if (m_flashCount % 2 == 0) {
            m_ckAssign->setStyleSheet("background-color: #FF80FF;");
        } else {
            m_ckAssign->setStyleSheet("background-color: palette(button);");
        }
    }
    // Flash error label
    if (m_lblErrIcon->isVisible() && m_flash) {
        m_flashCount++;
        m_lblErrIcon->setStyleSheet(m_flashCount % 2 == 0 ?
            "color: #8080FF; font-size: 16px;" :
            "color: #FF4444; font-size: 16px;");
    }
    // Flash paused buttons
    for (int i = 0; i < JINGLE_COUNT; ++i) {
        if (m_audio->isPaused(i)) {
            m_jingBtns[i]->setStyleSheet(m_flashCount % 2 == 0 ?
                "QPushButton{background:#C0FFC0;font-size:9px;}" :
                "QPushButton{background:#C0FFFF;font-size:9px;}");
        }
    }
}

void MainWindow::onTimer2()
{
    updateDisplayTime();

    if (!m_ckTmAnn->isChecked()) return;

    QTime now = QTime::currentTime();
    int nowMin = now.minute();
    int nowSec = now.second();
    int annMin = m_spMin->value();
    int annSec = m_spSec->value();
    int annDel = m_spDel->value();

    // Trigger time announce
    if (nowMin == annMin && nowSec == annSec) {
        m_audio->playTimeAnnounce();
        m_taJingWaiting = true;
        m_lblTaIcon->setVisible(true);
    }
    // Trigger delayed jingle after delay seconds
    if (m_taJingWaiting && nowMin == annMin && nowSec == annSec + annDel) {
        // Play the second TA jingle (already loaded via loadTimeAnnounce)
        m_taJingWaiting = false;
    }
}

void MainWindow::onTimer3()
{
    updateRemainingTime();
}

void MainWindow::onTimer5()
{
    // Poll playback states — raise buttons when finished
    for (int i = 0; i < JINGLE_COUNT; ++i) {
        if (m_audio->isStopped(i) && m_audio->jings[i].onAir) {
            m_audio->jings[i].onAir = false;
            m_jingBtns[i]->setPlaying(false);
        }
    }
    // Stream monitor
    if (m_ckStream->isChecked() && !m_audio->isStreamPlaying()) {
        m_ckStream->setChecked(false);
        m_lblConDisp->setText(m_lang->entry("mStrNoCo", tr("Not connected")));
    }
}

// ─── Display updates ──────────────────────────────────────────────────────────
void MainWindow::updateDisplayTime()
{
    QDateTime now = QDateTime::currentDateTime();
    m_lblTime->setText(now.toString(m_set->timeFormat));
    m_lblDate->setText(now.toString("yyyy/MM/dd"));
    // m_lblDay e m_lblWeek removidos do layout
}

void MainWindow::updateRemainingTime()
{
    // Find active jingle with most remaining time
    int trackIdx = m_vuTrackIdx;
    if (trackIdx < 0 || !m_audio->isPlaying(trackIdx)) {
        // Find any playing jingle
        for (int i = 0; i < JINGLE_COUNT; ++i) {
            if (m_audio->isPlaying(i)) { trackIdx = i; break; }
        }
    }

    if (trackIdx >= 0 && m_audio->isPlaying(trackIdx)) {
        qint64 dur  = m_audio->jingleDurationMs(trackIdx);
        qint64 pos  = m_audio->jinglePositionMs(trackIdx);
        qint64 rem  = dur - pos;
        if (rem < 0) rem = 0;

        double remSec = rem / 1000.0;
        int    remMin = (int)(remSec / 60);
        double remS   = remSec - remMin * 60;

        QString remStr;
        if (remMin > 0)
            remStr = QString("-%1:%2").arg(remMin).arg((int)remS, 2, 10, QChar('0'));
        else
            remStr = QString("-%1.%2")
                         .arg((int)remS, 2, 10, QChar('0'))
                         .arg((int)((remS - (int)remS) * 100), 2, 10, QChar('0'));

        m_lblRemTime->setText(remStr);
        m_lblRemTime->setAlignment(Qt::AlignCenter);
        // Cor: vermelho se <=10s, amarelo se <=30s, verde caso contrário (tamanho grande)
        if (remSec <= 10)
            m_lblRemTime->setStyleSheet("color: #ff3b30; font-size: 44px; max-height: 60px; font-weight: bold; font-family: 'Courier New'; background: transparent;");
        else if (remSec <= 30)
            m_lblRemTime->setStyleSheet("color: #ffd400; font-size: 44px; max-height: 60px; font-weight: bold; font-family: 'Courier New'; background: transparent;");
        else
            m_lblRemTime->setStyleSheet("color: #1aff1a; font-size: 44px; max-height: 60px; font-weight: bold; font-family: 'Courier New'; background: transparent;");
        m_lblWarnIcon->setVisible(remSec <= m_set->remainWarn && remSec > 0);
    } else {
        m_lblRemTime->setText("00.00");
        m_lblWarnIcon->setVisible(false);
    }
}

void MainWindow::updatePaletteDisplay()
{
    int row = m_lstPal->currentRow();
    int cnt = m_lstPal->count();

    m_lblCurPal->setText(row >= 0 && row < cnt ?
        m_lstPal->item(row)->text() : "");
    m_lblPrevPal->setText(row > 0 ?
        m_lstPal->item(row - 1)->text() : "");
    m_lblNextPal->setText(row >= 0 && row + 1 < cnt ?
        m_lstPal->item(row + 1)->text() : "");
}

// ─── VU levels ───────────────────────────────────────────────────────────────
void MainWindow::onVuLevels(float left, float right)
{
    m_vuL->setLevel(left);
    m_vuR->setLevel(right);
}

// ─── Settings / About dialogs ────────────────────────────────────────────────
void MainWindow::onSettingsClicked()
{
    if (!m_settingsDlg)
        m_settingsDlg = new SettingsDialog(m_set, m_lang, m_audio, this);
    if (m_settingsDlg->exec() == QDialog::Accepted) {
        m_set->save();
        applySettings();
        m_audio->init(m_set->device);
        m_audio->setMasterVolume(m_set->volume);
        ErrorLogger::instance().setEnabled(m_set->errorLog);
    }
}

void MainWindow::onAboutClicked()
{
    if (!m_aboutDlg)
        m_aboutDlg = new AboutDialog(this);
    m_aboutDlg->exec();
}

// ─── Helpers ─────────────────────────────────────────────────────────────────
QString MainWindow::formatJingleName(const QString &path, int maxLen) const
{
    if (path.isEmpty()) return "";
    QString name = QFileInfo(path).baseName();
    name.replace('_', ' ');
    name = name.left(1).toUpper() + name.mid(1).toLower();
    return name.left(maxLen);
}

void MainWindow::showError(const QString &msg, bool flash)
{
    m_flash = flash;
    m_lblErrIcon->setVisible(true);
    m_lblErrIcon->setToolTip(msg);
    ErrorLogger::instance().log("", msg);
}
