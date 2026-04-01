#include "remotecontrolserver.h"
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
#include <QCloseEvent>
#include <QScreen>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QSizePolicy>
#include <QFont>
#include <QPalette>
#include <QTextStream>
#include <cmath>

// ── Dark theme stylesheet ────────────────────────────────────────────────────
static const char *DARK_QSS = R"(
QMainWindow, QWidget {
    background-color: #1e1e1e;
    color: #e0e0e0;
}
QTabWidget::pane {
    border: 1px solid #444;
    background: #252525;
}
QTabBar::tab {
    background: #2d2d2d;
    color: #aaa;
    padding: 6px 16px;
    border: 1px solid #444;
    border-bottom: none;
}
QTabBar::tab:selected {
    background: #252525;
    color: #fff;
    border-top: 2px solid #00bb44;
}
QTabBar::tab:hover { background: #383838; color: #fff; }

QPushButton {
    background-color: #3a3a3a;
    color: #e0e0e0;
    border: 1px solid #555;
    border-radius: 4px;
    padding: 4px 10px;
}
QPushButton:hover  { background-color: #4a4a4a; }
QPushButton:pressed{ background-color: #252525; }
QPushButton:checked{ background-color: #00bb44; color: #000; border-color: #00ee55; }

QCheckBox { color: #e0e0e0; spacing: 6px; }
QCheckBox::indicator {
    width: 14px; height: 14px;
    background: #3a3a3a; border: 1px solid #666; border-radius: 3px;
}
QCheckBox::indicator:checked { background: #00bb44; border-color: #00ee55; }

QListWidget {
    background: #2a2a2a;
    color: #e0e0e0;
    border: 1px solid #444;
    alternate-background-color: #303030;
}
QListWidget::item:selected { background: #00884a; color: #fff; }
QListWidget::item:hover    { background: #383838; }

QLineEdit, QSpinBox {
    background: #2a2a2a;
    color: #e0e0e0;
    border: 1px solid #555;
    border-radius: 3px;
    padding: 3px 6px;
}
QLineEdit:focus, QSpinBox:focus { border-color: #00bb44; }

QSlider::groove:vertical {
    background: #3a3a3a;
    width: 6px;
    border-radius: 3px;
}
QSlider::handle:vertical {
    background: #00bb44;
    width: 14px; height: 14px;
    margin: -4px -4px;
    border-radius: 7px;
}
QSlider::sub-page:vertical { background: #00bb44; border-radius: 3px; }

QScrollBar:vertical {
    background: #2a2a2a; width: 8px;
}
QScrollBar::handle:vertical {
    background: #555; border-radius: 4px; min-height: 20px;
}

QMenu {
    background: #2a2a2a; color: #e0e0e0;
    border: 1px solid #555;
}
QMenu::item:selected { background: #00884a; }
QMenu::separator { background: #444; height: 1px; margin: 2px 0; }
)";

// ── Constructor ──────────────────────────────────────────────────────────────
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Inicializar settings antes de qualquer outra coisa
    m_appPath = QCoreApplication::applicationDirPath();
    m_set = new SettingsManager(m_appPath + "/jp.ini");
    if (!QFileInfo::exists(m_appPath + "/jp.ini"))
        m_set->applyDefaults(m_appPath);
    m_set->load();

    ErrorLogger::instance().setEnabled(m_set->errorLog);
    ErrorLogger::instance().setFilePath(m_appPath + "/Error.log");
    ErrorLogger::instance().log("", "Starting program...");

    m_pal  = new PaletteManager(m_appPath + "/palette.ini");
    m_lang = new LanguageManager(m_appPath + "/language.ini");
    m_lang->setLanguage(m_set->language);

    m_audio = new AudioEngine(this);
    m_audio->init(m_set->device);
    m_audio->setMasterVolume(m_set->volume);
    connect(m_audio, &AudioEngine::jingleFinished,      this, &MainWindow::onJingleFinished);
    connect(m_audio, &AudioEngine::vuLevelsUpdated,     this, &MainWindow::onVuLevels);

    applyDarkTheme();
    setupUi();
    applySettings();
    restoreWindowState();

    refreshPaletteList();
    if (!m_set->paletteIndex.isEmpty())
        loadPalette(m_set->paletteIndex);

    m_audio->loadTimeAnnounce(m_set->timeAnnouncer, "");

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

    // Remote control server (inicializado após m_set e m_audio)
    m_remoteServer = new RemoteControlServer(this);
    connect(m_remoteServer, &RemoteControlServer::playJingle, this, [this](int idx){
        if (idx >= 0 && idx < JINGLE_COUNT) playJingle(idx);
    });
    connect(m_remoteServer, &RemoteControlServer::stop, this, &MainWindow::stopAll);
    connect(m_remoteServer, &RemoteControlServer::pause, this, [this](){
        for (int i = JINGLE_COUNT-1; i >= 0; --i) {
            if (m_audio && m_audio->isPlaying(i)) { m_audio->pauseJingle(i); m_jingBtns[i]->setPaused(true); return; }
            if (m_audio && m_audio->isPaused(i))  { m_audio->resumeJingle(i); m_jingBtns[i]->setPlaying(true); return; }
        }
    });
    connect(m_remoteServer, &RemoteControlServer::autoMix, this, [this](){
        for (int i = 0; i < JINGLE_COUNT; ++i) {
            if (m_audio && m_audio->isPlaying(i)) { m_audio->startAutoMix(i, m_set->autoMixTime); break; }
        }
    });
    connect(m_remoteServer, &RemoteControlServer::autoRepeat, this, [this](){
        if (m_btnAutoRepeat) m_btnAutoRepeat->setChecked(!m_btnAutoRepeat->isChecked());
    });
    if (m_set->remoteOn) m_remoteServer->startServer(m_set->remotePort);

    setWindowTitle(QApplication::applicationName());
    updateDisplayTime();
    updatePaletteDisplay();
}

MainWindow::~MainWindow() { m_audio->free(); }

// ── Dark theme ────────────────────────────────────────────────────────────────
void MainWindow::applyDarkTheme()
{
    qApp->setStyleSheet(DARK_QSS);
    QPalette pal;
    pal.setColor(QPalette::Window,      QColor(0x1e,0x1e,0x1e));
    pal.setColor(QPalette::WindowText,  QColor(0xe0,0xe0,0xe0));
    pal.setColor(QPalette::Base,        QColor(0x2a,0x2a,0x2a));
    pal.setColor(QPalette::Button,      QColor(0x3a,0x3a,0x3a));
    pal.setColor(QPalette::ButtonText,  QColor(0xe0,0xe0,0xe0));
    pal.setColor(QPalette::Highlight,   QColor(0x00,0xbb,0x44));
    pal.setColor(QPalette::Text,        QColor(0xe0,0xe0,0xe0));
    qApp->setPalette(pal);
}

// ── Setup UI ──────────────────────────────────────────────────────────────────
void MainWindow::setupUi()
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    // ── Main vertical layout ─────────────────────────────────────────────────
    QVBoxLayout *vMain = new QVBoxLayout(central);
    vMain->setSpacing(4);
    vMain->setContentsMargins(8, 8, 8, 8);

    // ════════════════════════════════════════════════════════════════════════
    // TOP BAR: palette name | remaining time | clock | date
    // ════════════════════════════════════════════════════════════════════════
    QFrame *topFrame = new QFrame;
    topFrame->setStyleSheet("background:#1e1e1e;");
    topFrame->setMinimumHeight(100);
    QGridLayout *topGrid = new QGridLayout(topFrame);
    topGrid->setContentsMargins(6, 4, 6, 4);
    topGrid->setSpacing(4);

    // Palette name (top-left, bold green)
    m_lblPalName = new QLabel("PALETTE NAME");
    m_lblPalName->setStyleSheet(
        "color:#00ee44; font-size:18px; font-weight:bold; letter-spacing:2px;");

    // Prev / Next buttons
    m_btnPrev = new QPushButton("<");
    m_btnNext = new QPushButton(">");
    m_btnPrev->setFixedSize(72, 44);
    m_btnNext->setFixedSize(72, 44);
    m_btnPrev->setStyleSheet(
        "QPushButton{background:#3a3a3a;color:#ccc;font-size:16px;"
        "border:1px solid #555;border-radius:4px;}"
        "QPushButton:hover{background:#4a4a4a;}");
    m_btnNext->setStyleSheet(m_btnPrev->styleSheet());
    connect(m_btnPrev, &QPushButton::clicked, this, &MainWindow::onPrevPalette);
    connect(m_btnNext, &QPushButton::clicked, this, &MainWindow::onNextPalette);

    QHBoxLayout *hNav = new QHBoxLayout;
    hNav->addWidget(m_btnPrev);
    hNav->addWidget(m_btnNext);
    hNav->addStretch();

    // Remaining time (centre, big red)
    m_lblRemTime = new QLabel("-00.00");
    m_lblRemTime->setStyleSheet(
        "color:#00ff44; font-size:54px; font-weight:bold;"
        "font-family:'Courier New',monospace;"
        "background:transparent;"
        "qproperty-alignment: AlignCenter;");
    m_lblRemTime->setAlignment(Qt::AlignCenter);

    // Error / TA icons
    m_lblErrIcon = new QLabel("✗");
    m_lblErrIcon->setStyleSheet("color:#ff3333; font-size:22px; font-weight:bold;");
    m_lblErrIcon->setVisible(false);

    m_lblTaIcon = new QLabel("⏰");
    m_lblTaIcon->setStyleSheet("color:#00aaff; font-size:18px;");
    m_lblTaIcon->setVisible(false);

    QHBoxLayout *hRemIcons = new QHBoxLayout;
    hRemIcons->addStretch();
    hRemIcons->addWidget(m_lblRemTime);
    hRemIcons->addWidget(m_lblErrIcon);
    hRemIcons->addWidget(m_lblTaIcon);
    hRemIcons->addStretch();

    // Clock (top-right, big green)
    m_lblTime = new QLabel("00:00:00");
    m_lblTime->setStyleSheet(
        "color:#00ee44; font-size:18px; font-weight:bold;"
        "font-family:'Courier New',monospace;");
    m_lblTime->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // Date (below clock, green)
    m_lblDate = new QLabel("2026/03/27");
    m_lblDate->setStyleSheet(
        "color:#00ee44; font-size:18px; font-weight:bold;"
        "font-family:'Courier New',monospace;");
    m_lblDate->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QVBoxLayout *vClock = new QVBoxLayout;
    vClock->addWidget(m_lblTime);
    vClock->addWidget(m_lblDate);

    // Assemble top bar grid
    // col 0: pal name + nav | col 1: rem time | col 2: clock
    QVBoxLayout *vLeft = new QVBoxLayout;
    vLeft->addWidget(m_lblPalName);
    vLeft->addLayout(hNav);
    vLeft->addStretch();

    topGrid->addLayout(vLeft,       0, 0, Qt::AlignLeft | Qt::AlignTop);
    topGrid->addLayout(hRemIcons,   0, 1);
    topGrid->addLayout(vClock,      0, 2, Qt::AlignRight | Qt::AlignTop);
    topGrid->setColumnStretch(0, 4);
    topGrid->setColumnStretch(1, 6);
    topGrid->setColumnStretch(2, 5);

    vMain->addWidget(topFrame);

    // ════════════════════════════════════════════════════════════════════════
    // MIDDLE+BOTTOM: jingle grid, tabs, control buttons, VU meters (full height)
    // ════════════════════════════════════════════════════════════════════════

    // Jingle grid
    QWidget *gridWidget = new QWidget;
    gridWidget->setStyleSheet("background:#1e1e1e;");
    QGridLayout *grid = new QGridLayout(gridWidget);
    grid->setSpacing(3);
    grid->setContentsMargins(0, 0, 0, 0);

    // Estilo igual ao AUTOREPEAT/AUTOMIX (usado também nos botões de controle)
    QString gridBtnStyle =
        "QPushButton {"
        "  background: #3a3a3a;"
        "  color: #ccc;"
        "  font-size: 20px;"
        "  border: 1px solid #555;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover  { background: #4a4a4a; color: #fff; }"
        "QPushButton:pressed{ background: #252525; color: #fff; }"
        "QPushButton:checked{ background: #00bb44; color: #000; border-color: #00ee55; }";

    for (int i = 0; i < JINGLE_COUNT; ++i) {
        m_jingBtns[i] = new JingleButton(i, this);
        m_jingBtns[i]->setText(QString::number(i + 1));
        m_jingBtns[i]->setMinimumHeight(80);
        m_jingBtns[i]->setMaximumHeight(80);
        m_jingBtns[i]->setStyleSheet(gridBtnStyle);
        QFont f = m_jingBtns[i]->font();
        f.setPointSize(20);
        f.setBold(true);
        m_jingBtns[i]->setFont(f);
        connect(m_jingBtns[i], &QPushButton::clicked,
            this, [this, i](){ onJingleClicked(i); });
        connect(m_jingBtns[i], &JingleButton::rightClicked,
            this, &MainWindow::onJingleRightClicked);
        grid->addWidget(m_jingBtns[i], i / 5, i % 5);
    }
    // Layout horizontal do meio: grid de jingles + VU meters
    QHBoxLayout *hMid = new QHBoxLayout;
    hMid->setSpacing(4);
    hMid->setContentsMargins(0, 0, 0, 0);
    hMid->addWidget(gridWidget, 1);

    // VU meters column
    QVBoxLayout *vVu = new QVBoxLayout;
    vVu->setSpacing(2);
    vVu->setContentsMargins(2, 0, 2, 0);

    m_vuL = new VuMeter(Qt::Vertical);
    m_vuR = new VuMeter(Qt::Vertical);
    m_vuL->setObjectName("VuL");
    m_vuR->setObjectName("VuR");
    m_vuL->setFixedWidth(22);
    m_vuR->setFixedWidth(22);

    QHBoxLayout *hVu = new QHBoxLayout;
    hVu->setSpacing(2);
    hVu->addWidget(m_vuL);
    hVu->addWidget(m_vuR);

    vVu->addLayout(hVu, 1);

    // ════════════════════════════════════════════════════════════════════════
    // BOTTOM: tab panel (left) + big control buttons (right)
    // ════════════════════════════════════════════════════════════════════════
    QHBoxLayout *hBot = new QHBoxLayout;
    hBot->setSpacing(4);

    // ── Tab panel ────────────────────────────────────────────────────────────
    m_tabWidget = new QTabWidget;
    m_tabWidget->setMaximumHeight(220);

    // Tab 0 — Palettes
    {
        QWidget *w = new QWidget;
        QHBoxLayout *h = new QHBoxLayout(w);
        h->setContentsMargins(6, 6, 6, 6);

        QVBoxLayout *vLeft2 = new QVBoxLayout;

        QHBoxLayout *hAssign = new QHBoxLayout;
        m_ckAssign  = new QCheckBox(tr("Assign Jingle To Button"));
        m_txtAssign = new QLineEdit;
        m_txtAssign->setPlaceholderText("");
        m_txtAssign->setMaximumWidth(80);
        hAssign->addWidget(m_ckAssign);
        hAssign->addWidget(m_txtAssign);
        hAssign->addStretch();

        m_btSave = new QPushButton(tr("Save Current Palette"));
        m_btNew  = new QPushButton(tr("Delete Palette"));
        m_txtSave = new QLineEdit;
        m_txtSave->setPlaceholderText(tr("Palette name — press Enter"));
        m_txtSave->setVisible(false);

        vLeft2->addLayout(hAssign);
        vLeft2->addWidget(m_btSave);
        vLeft2->addWidget(m_txtSave);
        vLeft2->addWidget(m_btNew);
        vLeft2->addStretch();

        m_lstPal = new QListWidget;
        m_lstPal->setSortingEnabled(true);
        m_lstPal->setMinimumWidth(160);

        h->addLayout(vLeft2, 1);
        h->addWidget(m_lstPal, 1);

        connect(m_ckAssign, &QCheckBox::toggled,   this, &MainWindow::onAssignToggled);
        connect(m_btSave,   &QPushButton::clicked, this, &MainWindow::onSavePalette);
        connect(m_btNew,    &QPushButton::clicked, this, &MainWindow::onNewPalette);
        connect(m_txtSave,  &QLineEdit::returnPressed, this, [this](){
            QString name = m_txtSave->text().trimmed();
            if (!name.isEmpty()) {
                m_pal->savePalette(name, m_audio->jings);
                refreshPaletteList();
                auto items = m_lstPal->findItems(name, Qt::MatchExactly);
                if (!items.isEmpty()) m_lstPal->setCurrentItem(items.first());
                m_set->paletteIndex = name;
            }
            m_txtSave->setVisible(false);
        });
        connect(m_lstPal, &QListWidget::currentTextChanged,
                this, &MainWindow::onPaletteSelected);

        m_tabWidget->addTab(w, tr("Palettes"));
    }

    // Reset m_volSelIdx e restaura slider ao master quando o utilizador muda de tab
    connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int){
        if (m_volSelIdx >= 0) {
            m_volSelIdx = -1;
            m_slideVol->setValue(m_set->volume);
        }
    });

    // Tab 1 — Time Announce
    {
        QWidget *w = new QWidget;
        QGridLayout *g = new QGridLayout(w);
        g->setContentsMargins(8, 8, 8, 8);
        g->setSpacing(6);

        m_ckTmAnn   = new QCheckBox(tr("Time Announce"));
        m_btTmAnJin = new QCheckBox(tr("Select jingle to announce..."));
        m_btTmAnJin->setEnabled(false);

        m_spMin = new QSpinBox; m_spMin->setRange(0,59);
        m_spSec = new QSpinBox; m_spSec->setRange(0,59);
        m_spDel = new QSpinBox; m_spDel->setRange(0,59);

        g->addWidget(new QLabel(tr("Minute:")),        0, 0);
        g->addWidget(m_spMin,                          0, 1);
        g->addWidget(new QLabel(tr("Second:")),        1, 0);
        g->addWidget(m_spSec,                          1, 1);
        g->addWidget(new QLabel(tr("Jingle delay:")),  2, 0);
        g->addWidget(m_spDel,                          2, 1);
        g->addWidget(m_ckTmAnn,                        0, 2, 1, 2);
        g->addWidget(m_btTmAnJin,                      1, 2, 1, 2);
        g->setColumnStretch(3, 1);

        connect(m_ckTmAnn,   &QCheckBox::toggled, this, &MainWindow::onTimeAnnounceToggled);
        connect(m_btTmAnJin, &QCheckBox::toggled, this, &MainWindow::onSelectTaJingle);
        connect(m_spMin, QOverload<int>::of(&QSpinBox::valueChanged),
                this, [this](int v){ m_set->timeAnnMin = v; });
        connect(m_spSec, QOverload<int>::of(&QSpinBox::valueChanged),
                this, [this](int v){ m_set->timeAnnSec = v; });
        connect(m_spDel, QOverload<int>::of(&QSpinBox::valueChanged),
                this, [this](int v){ m_set->timeAnnDel = v; });

        m_tabWidget->addTab(w, tr("Time Announce"));
    }

    // Tab 2 — Settings
    {
        QWidget *w = new QWidget;
        QHBoxLayout *h = new QHBoxLayout(w);
        h->setContentsMargins(8, 8, 8, 8);
        h->setSpacing(12);

        // Volume slider — vertical, left side
        QVBoxLayout *vVol = new QVBoxLayout;
        QLabel *lblVolTitle = new QLabel(tr("Volume"));
        lblVolTitle->setAlignment(Qt::AlignCenter);
        lblVolTitle->setStyleSheet("color:#aaa; font-size:10px;");

        m_slideVol = new QSlider(Qt::Vertical);
        m_slideVol->setRange(0, 100);
        m_slideVol->setValue(m_set->volume);
        m_slideVol->setFixedWidth(30);
        m_slideVol->setMinimumHeight(100);
        m_slideVol->setToolTip(tr("Master Volume"));
        connect(m_slideVol, &QSlider::valueChanged,
                this, &MainWindow::onMasterVolumeChanged);

        m_lblVolVal = new QLabel("0 dB");
        m_lblVolVal->setAlignment(Qt::AlignCenter);
        m_lblVolVal->setStyleSheet("color:#00ee44; font-size:11px; font-weight:bold;");

        vVol->addWidget(lblVolTitle);
        vVol->addWidget(m_slideVol, 0, Qt::AlignHCenter);
        vVol->addWidget(m_lblVolVal);

        // Settings + About buttons
        QVBoxLayout *vBt = new QVBoxLayout;
        m_btSettings = new QPushButton(tr("Settings..."));
        m_btAbout    = new QPushButton(tr("About..."));
        m_btSettings->setMinimumHeight(36);
        m_btAbout->setMinimumHeight(36);
        vBt->addWidget(m_btSettings);
        vBt->addWidget(m_btAbout);
        vBt->addStretch();

        connect(m_btSettings, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);
        connect(m_btAbout,    &QPushButton::clicked, this, &MainWindow::onAboutClicked);

        h->addLayout(vVol);
        h->addLayout(vBt);
        h->addStretch();

        m_tabWidget->addTab(w, tr("Settings"));
    }

    hBot->addWidget(m_tabWidget, 3);

    // ── Big control buttons (2×2 grid, bottom right) ──────────────────────
    QWidget *ctrlWidget = new QWidget;
    ctrlWidget->setMaximumHeight(220);
    ctrlWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    // Layout externo: spacer (altura da tab bar) + grid de botões
    QVBoxLayout *ctrlOuter = new QVBoxLayout(ctrlWidget);
    ctrlOuter->setContentsMargins(0, 0, 0, 0);
    ctrlOuter->setSpacing(0);

    m_ctrlTopSpacer = new QWidget;
    m_ctrlTopSpacer->setFixedHeight(0); // definido em applySettings após show
    ctrlOuter->addWidget(m_ctrlTopSpacer);

    QWidget *ctrlButtons = new QWidget;
    ctrlButtons->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    ctrlOuter->addWidget(ctrlButtons, 1);

    QGridLayout *ctrlGrid = new QGridLayout(ctrlButtons);
    ctrlGrid->setSpacing(5);
    ctrlGrid->setContentsMargins(0, 0, 0, 0);

    m_btnAutoRepeat = new QPushButton("AUTOREPEAT");
    m_btnPause      = new QPushButton("PAUSE");
    m_btnAutoMix    = new QPushButton("AUTOMIX");
    m_btnStop       = new QPushButton("STOP");


    // Usar exatamente o mesmo tamanho e fonte dos botões da grid 5x6
    int gridBtnFontSize = 20;

    // PAUSE: igual ao grid, só muda cor
    QString pauseStyle =
        "QPushButton {"
        "  background: #5a5a1f; color: #fff;"
        "  border: 1px solid #888; border-radius: 4px;"
        "  font-size: 20px; font-weight: bold;"
        "}"
        "QPushButton:hover  { background: #7a7a2f; color: #fff; }"
        "QPushButton:checked { background: #ffff66; color: #222; border-color: #cccc33; }";

    // STOP: igual ao grid, só muda cor
    QString stopStyle =
        "QPushButton {"
        "  background: #661a1a; color: #fff;"
        "  border: 1px solid #a33; border-radius: 4px;"
        "  font-size: 20px; font-weight: bold;"
        "}"
        "QPushButton:hover  { background: #992a2a; color: #fff; }"
        "QPushButton:checked { background: #ff3333; color: #fff; border-color: #b71c1c; }";

    m_btnAutoRepeat->setStyleSheet(gridBtnStyle);
    m_btnAutoMix->setStyleSheet(gridBtnStyle);
    m_btnPause->setStyleSheet(pauseStyle);
    m_btnStop->setStyleSheet(stopStyle);

    m_btnAutoRepeat->setCheckable(true);
    m_btnAutoMix->setCheckable(true);

    // Ajustar tamanho igual ao grid (mínimo igual ao dos jingles)
    m_btnAutoRepeat->setMinimumHeight(80);
    m_btnAutoMix->setMinimumHeight(80);
    m_btnPause->setMinimumHeight(80);
    m_btnStop->setMinimumHeight(80);

    QFont ctrlFont = m_btnAutoRepeat->font();
    ctrlFont.setPointSize(gridBtnFontSize);
    ctrlFont.setBold(true);
    m_btnAutoRepeat->setFont(ctrlFont);
    m_btnAutoMix->setFont(ctrlFont);
    m_btnPause->setFont(ctrlFont);
    m_btnStop->setFont(ctrlFont);

    ctrlGrid->addWidget(m_btnAutoRepeat, 0, 0);
    ctrlGrid->addWidget(m_btnPause,      0, 1);
    ctrlGrid->addWidget(m_btnAutoMix,    1, 0);
    ctrlGrid->addWidget(m_btnStop,       1, 1);
    ctrlGrid->setRowStretch(0, 1);
    ctrlGrid->setRowStretch(1, 1);

    connect(m_btnAutoRepeat, &QPushButton::clicked, this, &MainWindow::onBtnAutoRepeat);
    connect(m_btnPause,      &QPushButton::clicked, this, &MainWindow::onBtnPause);
    connect(m_btnAutoMix,    &QPushButton::clicked, this, &MainWindow::onBtnAutoMix);
    connect(m_btnStop,       &QPushButton::clicked, this, &MainWindow::onBtnStop);

    hBot->addWidget(ctrlWidget, 2);

    // Layout externo: conteúdo (grid + painel inferior) à esquerda, VU à direita
    // O vVu está fora do hMid para poder descer até ao topo do botão STOP
    QHBoxLayout *hOuter = new QHBoxLayout;
    hOuter->setSpacing(8);
    hOuter->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout *vContent = new QVBoxLayout;
    vContent->setSpacing(4);
    vContent->setContentsMargins(0, 0, 0, 0);
    vContent->addLayout(hMid, 1);
    vContent->addLayout(hBot);

    hOuter->addLayout(vContent, 1);
    hOuter->addLayout(vVu);

    vMain->addLayout(hOuter, 1);

    setupContextMenu();
}

void MainWindow::setupContextMenu()
{
    m_ctxMenu  = new QMenu(this);
    m_actPlay  = m_ctxMenu->addAction(tr("Play from start"));
    m_actPause = m_ctxMenu->addAction(tr("Pause / Resume"));
    m_actStop  = m_ctxMenu->addAction(tr("Stop"));
    m_ctxMenu->addSeparator();
    m_actAssign = m_ctxMenu->addAction(tr("Assign..."));
    m_actClear  = m_ctxMenu->addAction(tr("Clear"));
    m_ctxMenu->addSeparator();
    m_actLoop   = m_ctxMenu->addAction(tr("Toggle Loop"));
    m_actVolume = m_ctxMenu->addAction(tr("Set Volume..."));
    m_actTa     = m_ctxMenu->addAction(tr("Use as Time Announce Jingle"));

    connect(m_actPlay,   &QAction::triggered, this, &MainWindow::onMenuPlay);
    connect(m_actPause,  &QAction::triggered, this, &MainWindow::onMenuPause);
    connect(m_actStop,   &QAction::triggered, this, &MainWindow::onMenuStop);
    connect(m_actAssign, &QAction::triggered, this, &MainWindow::onMenuAssign);
    connect(m_actClear,  &QAction::triggered, this, &MainWindow::onMenuClear);
    connect(m_actLoop,   &QAction::triggered, this, &MainWindow::onMenuLoop);
    connect(m_actVolume, &QAction::triggered, this, &MainWindow::onMenuVolume);
    connect(m_actTa,     &QAction::triggered, this, &MainWindow::onMenuTimeAnnounce);
}

// ── Apply / Save settings ────────────────────────────────────────────────────
void MainWindow::applySettings()
{
    setWindowFlag(Qt::WindowStaysOnTopHint, m_set->alwaysOnTop);
    show();
    m_slideVol->setValue(m_set->volume);
    if (m_spMin) m_spMin->setValue(m_set->timeAnnMin);
    if (m_spSec) m_spSec->setValue(m_set->timeAnnSec);
    if (m_spDel) m_spDel->setValue(m_set->timeAnnDel);
    m_tabWidget->setCurrentIndex(qBound(0, m_set->activeTab, 2));

    // Alinhar o topo dos botões de controlo com a área de conteúdo das tabs
    if (m_ctrlTopSpacer && m_tabWidget)
        m_ctrlTopSpacer->setFixedHeight(m_tabWidget->tabBar()->height());
}

void MainWindow::restoreWindowState()
{
    QScreen *scr = QGuiApplication::primaryScreen();
    QRect avail  = scr->availableGeometry();
    int w = m_set->winWidth  > 400 ? m_set->winWidth  : 1000;
    int h = m_set->winHeight > 300 ? m_set->winHeight : 700;
    resize(w, h);
    int x = m_set->posLeft, y = m_set->posTop;
    if (x < 0 || y < 0 || x+w > avail.right() || y+h > avail.bottom()) {
        x = avail.center().x() - w/2;
        y = avail.center().y() - h/2;
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
    m_set->winState  = isMaximized() ? 2 : 0;
    m_set->activeTab = m_tabWidget->currentIndex();
    m_set->volume    = m_slideVol->value();
    m_set->timeAnnMin = m_spMin->value();
    m_set->timeAnnSec = m_spSec->value();
    m_set->timeAnnDel = m_spDel->value();
    if (m_lstPal->currentItem())
        m_set->paletteIndex = m_lstPal->currentItem()->text();
    m_set->save();
}

// ── Close / Key events ────────────────────────────────────────────────────────
void MainWindow::closeEvent(QCloseEvent *e)
{
    saveWindowState();

    bool playbackActive = false;
    for (int i = 0; i < JINGLE_COUNT; ++i) {
        if (m_audio && m_audio->isPlaying(i)) {
            playbackActive = true;
            break;
        }
    }

    if (playbackActive) {
        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle(tr("Playback Active"));
        msgBox.setText(tr("Music is playing!\n\nDo you want to lose your job and ruin your career?"));
        QPushButton *stopAndCloseBtn = msgBox.addButton(tr("Yes and Close"), QMessageBox::DestructiveRole);
        msgBox.addButton(tr("Cancel"), QMessageBox::RejectRole);
        msgBox.exec();

        if (msgBox.clickedButton() != stopAndCloseBtn) {
            e->ignore();
            return;
        }
    }

    if (m_remoteServer)
        m_remoteServer->stopServer();

    stopAll();
    m_audio->free();
    ErrorLogger::instance().log("", "Program normal exit");
    e->accept();
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
    int jingleIdx = -1;

    // Map keys to jingle indices
    // Keys 1-5 → Jingles 0-4
    if (e->key() >= Qt::Key_1 && e->key() <= Qt::Key_5)
        jingleIdx = e->key() - Qt::Key_1;
    // Keys Q,W,E,R,T → Jingles 5-9
    else if (e->key() == Qt::Key_Q) jingleIdx = 5;
    else if (e->key() == Qt::Key_W) jingleIdx = 6;
    else if (e->key() == Qt::Key_E) jingleIdx = 7;
    else if (e->key() == Qt::Key_R) jingleIdx = 8;
    else if (e->key() == Qt::Key_T) jingleIdx = 9;
    // Keys A,S,D,F,G → Jingles 10-14
    else if (e->key() == Qt::Key_A) jingleIdx = 10;
    else if (e->key() == Qt::Key_S) jingleIdx = 11;
    else if (e->key() == Qt::Key_D) jingleIdx = 12;
    else if (e->key() == Qt::Key_F) jingleIdx = 13;
    else if (e->key() == Qt::Key_G) jingleIdx = 14;
    // Keys Z,X,C,V,B → Jingles 15-19
    else if (e->key() == Qt::Key_Z) jingleIdx = 15;
    else if (e->key() == Qt::Key_X) jingleIdx = 16;
    else if (e->key() == Qt::Key_C) jingleIdx = 17;
    else if (e->key() == Qt::Key_V) jingleIdx = 18;
    else if (e->key() == Qt::Key_B) jingleIdx = 19;

    // If mapped to a jingle, click it
    if (jingleIdx >= 0 && jingleIdx < JINGLE_COUNT && m_jingBtns[jingleIdx]) {
        m_jingBtns[jingleIdx]->click();
        e->accept();
        return;
    }

    switch (e->key()) {
    case Qt::Key_M:
        m_btnAutoRepeat->setChecked(!m_btnAutoRepeat->isChecked());
        e->accept();
        return;
    case Qt::Key_Space:
        if (m_btnPause) {
            m_btnPause->click();
            e->accept();
            return;
        }
        break;
    }

    QMainWindow::keyPressEvent(e);
}

// ── Jingle operations ─────────────────────────────────────────────────────────
void MainWindow::onJingleClicked(int idx)
{
    if (m_ckAssign->isChecked()) { assignJingle(idx); return; }
    if (m_btTmAnJin->isChecked() && !m_audio->jings[idx].path.isEmpty()) {
        m_audio->loadTimeAnnounce(m_set->timeAnnouncer, m_audio->jings[idx].path);
        m_btTmAnJin->setChecked(false);
        return;
    }
    playJingle(idx);
}

void MainWindow::assignJingle(int idx)
{
    if (m_audio->isPlaying(idx)) return;
    m_ckAssign->setChecked(false);
    QString path = QFileDialog::getOpenFileName(this,
        tr("Assign Jingle to Button"), "",
        tr("Audio Files (*.wav *.mp3 *.mp2 *.mp1 *.mpa *.ogg *.flac *.aiff *.aif *.m4a *.aac)"));
    if (path.isEmpty()) return;
    m_audio->jings[idx].path = path;
    m_audio->jings[idx].loop = false;
    m_audio->loadJingle(idx, path);
    m_jingBtns[idx]->setText(formatJingleName(path));
    m_jingBtns[idx]->setLoopIndicatorVisible(false);
    m_jingBtns[idx]->setPlaying(false);
}

void MainWindow::playJingle(int idx)
{
    if (m_audio->jings[idx].path.isEmpty()) return;

    if (m_audio->isPlaying(idx)) {
        if (m_btnAutoRepeat->isChecked())
            m_audio->seekJingle(idx, 0);
        else {
            m_audio->stopJingle(idx);
            m_jingBtns[idx]->setPlaying(false);
        }
    } else {
        m_audio->playJingle(idx);
        m_jingBtns[idx]->setPlaying(true);
        m_jingBtns[idx]->setChecked(true);
        m_vuTrackIdx = idx;
        if (m_btnAutoMix->isChecked())
            m_audio->startAutoMix(idx, m_set->autoMixTime);
    }
}

void MainWindow::stopAll()
{
    for (int i = 0; i < JINGLE_COUNT; ++i) {
        m_audio->stopJingle(i);
        m_jingBtns[i]->setPlaying(false);
        m_jingBtns[i]->setChecked(false);
    }
    m_vuTrackIdx = -1;
}

void MainWindow::onJingleFinished(int idx)
{
    m_jingBtns[idx]->setPlaying(false);
    m_jingBtns[idx]->setChecked(false);
    if (m_vuTrackIdx == idx) m_vuTrackIdx = -1;
}

// ── Context menu ──────────────────────────────────────────────────────────────
void MainWindow::onJingleRightClicked(int idx)
{
    m_ctxIdx = idx;
    bool has = !m_audio->jings[idx].path.isEmpty();
    m_actPlay->setEnabled(has);
    m_actPause->setEnabled(has);
    m_actStop->setEnabled(has && (m_audio->isPlaying(idx)||m_audio->isPaused(idx)));
    m_actClear->setEnabled(has);
    m_actLoop->setEnabled(has && !m_audio->isPlaying(idx));
    m_actVolume->setEnabled(has);
    m_actTa->setEnabled(has);
    m_ctxMenu->exec(QCursor::pos());
}

void MainWindow::onMenuPlay()
{
    if (m_ctxIdx < 0) return;
    if (m_audio->isPaused(m_ctxIdx)) m_audio->seekJingle(m_ctxIdx, 0);
    m_audio->playJingle(m_ctxIdx);
    m_jingBtns[m_ctxIdx]->setPlaying(true);
    m_jingBtns[m_ctxIdx]->setChecked(true);
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
    m_jingBtns[m_ctxIdx]->setChecked(false);
}
void MainWindow::onMenuAssign() { if (m_ctxIdx >= 0) assignJingle(m_ctxIdx); }
void MainWindow::onMenuClear()
{
    if (m_ctxIdx < 0 || m_audio->jings[m_ctxIdx].path.isEmpty()) return;
    if (QMessageBox::question(this, "Jingle Palette Pro",
            tr("Clear this jingle button?"),
            QMessageBox::Yes|QMessageBox::No) != QMessageBox::Yes) return;
    m_audio->stopJingle(m_ctxIdx);
    m_audio->jings[m_ctxIdx].path   = "";
    m_audio->jings[m_ctxIdx].color  = QColor(0xF0, 0xF0, 0xF0);
    m_audio->jings[m_ctxIdx].volume = 100;
    m_audio->jings[m_ctxIdx].loop   = false;
    m_audio->jings[m_ctxIdx].vuL    = 0.f;
    m_audio->jings[m_ctxIdx].vuR    = 0.f;
    m_audio->jings[m_ctxIdx].inDebt = false;
    m_audio->loadJingle(m_ctxIdx, "");
    m_jingBtns[m_ctxIdx]->setText(QString::number(m_ctxIdx+1));
    m_jingBtns[m_ctxIdx]->setPlaying(false);
    m_jingBtns[m_ctxIdx]->setChecked(false);
    m_jingBtns[m_ctxIdx]->setLoopIndicatorVisible(false);
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
    m_tabWidget->setCurrentIndex(2);
    m_volSelIdx = m_ctxIdx;
    m_slideVol->setValue(m_audio->jings[m_ctxIdx].volume);
}
void MainWindow::onMenuTimeAnnounce()
{
    if (m_ctxIdx < 0) return;
    m_audio->loadTimeAnnounce(m_set->timeAnnouncer, m_audio->jings[m_ctxIdx].path);
    m_btTmAnJin->setChecked(true);
}

// ── Palette operations ────────────────────────────────────────────────────────
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
    m_pal->loadPalette(name, m_audio->jings);
    for (int i = 0; i < JINGLE_COUNT; ++i) {
        if (!m_audio->isPlaying(i)) {
            m_audio->loadJingle(i, m_audio->jings[i].path);
            m_jingBtns[i]->setText(m_audio->jings[i].path.isEmpty() ?
                QString::number(i+1) : formatJingleName(m_audio->jings[i].path));
            m_jingBtns[i]->setPlaying(false);
            m_jingBtns[i]->setChecked(false);
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
}

void MainWindow::onAssignToggled(bool on)
{
    m_ckAssign->setStyleSheet(on ?
        "QCheckBox{color:#00ff88;}" : "QCheckBox{color:#e0e0e0;}");
}

void MainWindow::onSavePalette()
{
    // Se já houver um nome selecionado na lista, salva imediatamente
    if (m_lstPal && m_lstPal->currentItem() && !m_lstPal->currentItem()->text().trimmed().isEmpty()) {
        QString name = m_lstPal->currentItem()->text().trimmed();
        m_pal->savePalette(name, m_audio->jings);
        refreshPaletteList();
        auto items = m_lstPal->findItems(name, Qt::MatchExactly);
        if (!items.isEmpty()) m_lstPal->setCurrentItem(items.first());
        m_set->paletteIndex = name;
    } else {
        // Senão, pede nome novo
        m_txtSave->setVisible(true);
        m_txtSave->setFocus();
        m_txtSave->clear();
    }
}

void MainWindow::onNewPalette()
{
    // Apagar a palette seleccionada (se houver)
    QString currentName;
    if (m_lstPal && m_lstPal->currentItem())
        currentName = m_lstPal->currentItem()->text();

    if (!currentName.isEmpty()) {
        if (QMessageBox::question(this, tr("Delete Palette"),
                tr("Delete palette \"%1\"?").arg(currentName),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
            return;
        m_pal->deletePalette(currentName);
        m_set->paletteIndex = "";
    }

    stopAll();
    for (int i = 0; i < JINGLE_COUNT; ++i) {
        m_audio->jings[i].path   = "";
        m_audio->jings[i].color  = QColor(0xF0, 0xF0, 0xF0);
        m_audio->jings[i].volume = 100;
        m_audio->jings[i].loop   = false;
        m_audio->jings[i].vuL    = 0.f;
        m_audio->jings[i].vuR    = 0.f;
        m_audio->jings[i].inDebt = false;
        m_audio->loadJingle(i, "");
        if (m_jingBtns[i]) {
            m_jingBtns[i]->setText(QString::number(i+1));
            m_jingBtns[i]->setPlaying(false);
            m_jingBtns[i]->setChecked(false);
            m_jingBtns[i]->setLoopIndicatorVisible(false);
        }
    }

    refreshPaletteList();
    if (m_lstPal) m_lstPal->setCurrentRow(-1);
    updatePaletteDisplay();
}

void MainWindow::onPrevPalette()
{
    int row = m_lstPal->currentRow();
    if (row > 0) {
        m_lstPal->setCurrentRow(row-1);
        onPaletteSelected(m_lstPal->currentItem()->text());
    }
}

void MainWindow::onNextPalette()
{
    int row = m_lstPal->currentRow();
    if (row < m_lstPal->count()-1) {
        m_lstPal->setCurrentRow(row+1);
        onPaletteSelected(m_lstPal->currentItem()->text());
    }
}

// ── Time Announce ─────────────────────────────────────────────────────────────
void MainWindow::onTimeAnnounceToggled(bool on)
{
    m_btTmAnJin->setEnabled(on);
    m_lblTaIcon->setVisible(on);
    if (!on) m_audio->stopTimeAnnounce();
}

void MainWindow::onSelectTaJingle(bool on)
{
    m_btTmAnJin->setStyleSheet(on ?
        "QCheckBox{color:#ffaaaa;}" : "QCheckBox{color:#e0e0e0;}");
}

// ── Volume ────────────────────────────────────────────────────────────────────
void MainWindow::onMasterVolumeChanged(int val)
{
    if (m_volSelIdx >= 0) {
        m_audio->setJingleVolume(m_volSelIdx, val);
    } else {
        m_audio->setMasterVolume(val);
        m_set->volume = val;
    }
    if (val <= 0)
        m_lblVolVal->setText("-∞ dB");
    else
        m_lblVolVal->setText(QString::number((int)(20.0 * std::log10(val / 100.0))) + " dB");
}

// ── Big control buttons ───────────────────────────────────────────────────────
void MainWindow::onBtnAutoRepeat()
{
    // checkable — state handled by Qt
}
void MainWindow::onBtnPause()
{
    // Pause/resume the most recently active jingle
    bool changed = false;
    for (int i = JINGLE_COUNT-1; i >= 0; --i) {
        if (m_audio->isPlaying(i)) {
            m_audio->pauseJingle(i);
            m_jingBtns[i]->setPaused(true);
            changed = true;
            break;
        }
        if (m_audio->isPaused(i)) {
            m_audio->resumeJingle(i);
            m_jingBtns[i]->setPlaying(true);
            changed = true;
            break;
        }
    }
}
void MainWindow::onBtnAutoMix()
{
    // checkable — state handled by Qt
}
void MainWindow::onBtnStop()
{
    stopAll();
}

// ── Settings / About ──────────────────────────────────────────────────────────
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
        // Remote server: start/stop conforme settings
        if (m_set->remoteOn) {
            m_remoteServer->startServer(m_set->remotePort);
        } else {
            m_remoteServer->stopServer();
        }
    }
}

void MainWindow::onAboutClicked()
{
    if (!m_aboutDlg) m_aboutDlg = new AboutDialog(this);
    m_aboutDlg->exec();
}

// ── Timers ────────────────────────────────────────────────────────────────────
void MainWindow::onTimer1()
{
    m_flashCount++;
    // Flash assign checkbox
    if (m_ckAssign->isChecked()) {
        m_ckAssign->setStyleSheet(m_flashCount % 2 == 0 ?
            "QCheckBox{color:#00ff88;}" : "QCheckBox{color:#e0e0e0;}");
    }
    // Flash paused buttons (grid)
    for (int i = 0; i < JINGLE_COUNT; ++i) {
        if (m_audio->isPaused(i)) {
            // Pisca entre cinza e amarelo
            m_jingBtns[i]->setStyleSheet(m_flashCount % 2 == 0 ?
                "QPushButton{background:#ffff66;color:#222;border:1px solid #cccc33;border-radius:4px;font-size:20px;font-weight:bold;}" :
                "QPushButton{background:#3a3a3a;color:#ccc;border:1px solid #555;border-radius:4px;font-size:20px;font-weight:bold;}");
        } else if (m_audio->isPlaying(i)) {
            // Sempre verde quando tocando
            m_jingBtns[i]->setStyleSheet("QPushButton{background:#00bb44;color:#000;border:1px solid #00ee55;border-radius:4px;font-size:20px;font-weight:bold;}");
        } else {
            // Estado normal (cinza)
            m_jingBtns[i]->setStyleSheet("QPushButton{background:#3a3a3a;color:#ccc;border:1px solid #555;border-radius:4px;font-size:20px;font-weight:bold;}");
        }
    }

    // Sincronizar PAUSE button (fora da grid) com estado global de pause
    if (m_btnPause) {
        bool algumPausado = false;
        for (int i = 0; i < JINGLE_COUNT; ++i) {
            if (m_audio->isPaused(i)) { algumPausado = true; break; }
        }
        m_btnPause->setChecked(algumPausado);
        if (algumPausado) {
            // Sempre amarelo forte quando activo
            m_btnPause->setStyleSheet("QPushButton { background: #ffff66; color: #222; border: 1px solid #cccc33; border-radius: 4px; font-size: 20px; font-weight: bold; }");
        } else {
            // Sempre amarelo escuro quando não está ativo
            m_btnPause->setStyleSheet("QPushButton { background: #5A5A1F; color: #fff; border: 1px solid #888; border-radius: 4px; font-size: 20px; font-weight: bold; }");
        }
    }
}

void MainWindow::onTimer2()
{
    updateDisplayTime();

    if (!m_ckTmAnn->isChecked()) return;
    QTime now = QTime::currentTime();
    if (now.minute() == m_spMin->value() && now.second() == m_spSec->value()) {
        m_audio->playTimeAnnounce();
        m_taJingWaiting = true;
        m_lblTaIcon->setVisible(true);
    }
    if (m_taJingWaiting) {
        QTime target = QTime(now.hour(), m_spMin->value(), m_spSec->value())
                           .addSecs(m_spDel->value());
        if (now >= target)
            m_taJingWaiting = false;
    }
}

void MainWindow::onTimer3()
{
    updateRemainingTime();
}

void MainWindow::onTimer5()
{
    for (int i = 0; i < JINGLE_COUNT; ++i) {
        if (m_audio->isStopped(i) && m_audio->jings[i].onAir) {
            m_audio->jings[i].onAir = false;
            m_jingBtns[i]->setPlaying(false);
            m_jingBtns[i]->setChecked(false);
        }
    }
}

// ── Display updates ───────────────────────────────────────────────────────────
void MainWindow::updateDisplayTime()
{
    QDateTime now = QDateTime::currentDateTime();
    m_lblTime->setText(now.toString(m_set->timeFormat));
    m_lblDate->setText(now.toString(m_set->dateFormat));
}

void MainWindow::updateRemainingTime()
{
    int trackIdx = m_vuTrackIdx;
    if (trackIdx < 0 || !m_audio->isPlaying(trackIdx)) {
        for (int i = 0; i < JINGLE_COUNT; ++i)
            if (m_audio->isPlaying(i)) { trackIdx = i; break; }
    }

    if (trackIdx >= 0 && m_audio->isPlaying(trackIdx)) {
        qint64 rem  = m_audio->jingleDurationMs(trackIdx)
                    - m_audio->jinglePositionMs(trackIdx);
        if (rem < 0) rem = 0;
        double remSec = rem / 1000.0;
        int    remMin = (int)(remSec / 60);
        double remS   = remSec - remMin * 60;

        QString txt;
        if (remMin > 0)
            txt = QString("-%1:%2")
                      .arg(remMin)
                      .arg((int)remS, 2, 10, QChar('0'));
        else
            txt = QString("-%1.%2")
                      .arg((int)remS, 2, 10, QChar('0'))
                      .arg((int)((remS-(int)remS)*100), 2, 10, QChar('0'));

        m_lblRemTime->setText(txt);

        // Default: green, <=30s: yellow, <=10s: red
        QString color = "#00ff44";
        if (remSec <= 10.0 && remSec > 0) {
            color = "#ff3333";
        } else if (remSec <= 30.0 && remSec > 0) {
            color = "#ffe066";
        }
        m_lblRemTime->setStyleSheet(
            QString("color:%1; font-size:52px; font-weight:bold;"
                    "font-family:'Courier New',monospace;")
                .arg(color));
        m_lblErrIcon->setVisible(remSec <= 30.0 && remSec > 0);
    } else {
        m_lblRemTime->setText("-00.00");
        m_lblErrIcon->setVisible(false);
    }
}

void MainWindow::updatePaletteDisplay()
{
    int row = m_lstPal ? m_lstPal->currentRow() : -1;
    int cnt = m_lstPal ? m_lstPal->count() : 0;
    if (row >= 0 && row < cnt && m_lstPal->item(row))
        m_lblPalName->setText(m_lstPal->item(row)->text().toUpper());
    else
        m_lblPalName->setText("PALETTE NAME");
}

// ── VU levels ─────────────────────────────────────────────────────────────────
void MainWindow::onVuLevels(float left, float right)
{
    m_vuL->setLevel(left);
    m_vuR->setLevel(right);
}

// ── Helpers ───────────────────────────────────────────────────────────────────
QString MainWindow::formatJingleName(const QString &path, int maxLen) const
{
    if (path.isEmpty()) return "";
    QString name = QFileInfo(path).baseName().replace('_', ' ');
    name = name.left(1).toUpper() + name.mid(1).toLower();
    return name.left(maxLen);
}
