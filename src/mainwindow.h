#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <QTabWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSlider>
#include <QMenu>
#include <QAction>
#include <QSpinBox>
#include <QGroupBox>
#include <QFrame>
#include <array>

#include "globals.h"
#include "audioengine.h"
#include "palettemanager.h"
#include "settingsmanager.h"
#include "languagemanager.h"
#include "vumeter.h"
#include "jinglebutton.h"

class SettingsDialog;
class AboutDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void loadPalette(const QString &name);
    void refreshPaletteList();

protected:
    void closeEvent(QCloseEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void resizeEvent(QResizeEvent *) override;

private slots:
    // Jingle buttons
    void onJingleClicked(int idx);
    void onJingleRightClicked(int idx);

    // Timers
    void onTimer1();    // flash / UI update
    void onTimer2();    // clock + time announce trigger
    void onTimer3();    // VU meter + remaining time display
    void onTimer5();    // playback state polling

    // Palette tab
    void onAssignToggled(bool on);
    void onSavePalette();
    void onNewPalette();
    void onPaletteSelected(const QString &name);

    // Stream tab
    void onPlayStreamToggled(bool on);
    void onStreamStatusChanged(const QString &status);

    // Time Announce tab
    void onTimeAnnounceToggled(bool on);
    void onSelectTaJingle(bool on);

    // Settings tab
    void onVolumeChanged(int val);

    // Context menu actions
    void onMenuPlay();
    void onMenuPause();
    void onMenuStop();
    void onMenuAssign();
    void onMenuClear();
    void onMenuLoop();
    void onMenuVolume();
    void onMenuTimeAnnounce();

    // Toolbar / buttons
    void onSettingsClicked();
    void onAboutClicked();

    // Audio engine
    void onJingleFinished(int idx);
    void onVuLevels(float left, float right);

    // Navigation display
    void onPrevPalette();
    void onNextPalette();

private:
    void setupUi();
    void setupDisplay();
    void setupJingleGrid();
    void setupTabPanel();
    void setupVuMeter();
    void setupContextMenu();

    void applySettings();
    void saveWindowState();
    void restoreWindowState();

    void assignJingle(int idx);
    void playJingle(int idx);
    void stopAll();

    void showError(const QString &msg, bool flash = false);
    void updateDisplayTime();
    void updateRemainingTime();
    void updatePaletteDisplay();

    QString formatJingleName(const QString &path, int maxLen = 60) const;

    // ---- Audio + data ----
    AudioEngine      *m_audio   = nullptr;
    PaletteManager   *m_pal     = nullptr;
    SettingsManager  *m_set     = nullptr;
    LanguageManager  *m_lang    = nullptr;

    // ---- Timers ----
    QTimer *m_timer1 = nullptr;   // 500ms  — flash
    QTimer *m_timer2 = nullptr;   // 1000ms — clock
    QTimer *m_timer3 = nullptr;   // 80ms   — VU + remaining
    QTimer *m_timer5 = nullptr;   // 200ms  — button state poll

    // ---- Display panel ----
    QFrame   *m_displayFrame = nullptr;
    QLabel   *m_lblRemTime   = nullptr;   // remaining time  -02.57
    QLabel   *m_lblWarnIcon  = nullptr;   // warning flash
    QLabel   *m_lblTaIcon    = nullptr;   // time announce active
    QLabel   *m_lblErrIcon   = nullptr;   // error
    QLabel   *m_lblTime      = nullptr;   // 18:44:14
    QLabel   *m_lblPrevPal   = nullptr;   // previous palette name
    QLabel   *m_lblCurPal    = nullptr;   // current palette (bold)
    QLabel   *m_lblNextPal   = nullptr;   // next palette name
    QLabel   *m_lblDay       = nullptr;   // Saturday
    QLabel   *m_lblDate      = nullptr;   // January 03, 2004
    QLabel   *m_lblWeek      = nullptr;   // week number
    QPushButton *m_btnPrev   = nullptr;   // ◄
    QPushButton *m_btnNext   = nullptr;   // ►

    // ---- Jingle grid ----
    std::array<JingleButton*, JINGLE_COUNT> m_jingBtns = {};

    // ---- VU meters ----
    VuMeter *m_vuL = nullptr;
    VuMeter *m_vuR = nullptr;

    // ---- Tab panel ----
    QTabWidget  *m_tabWidget  = nullptr;

    // Palettes tab
    QCheckBox   *m_ckAssign   = nullptr;
    QPushButton *m_btSave     = nullptr;
    QLineEdit   *m_txtSave    = nullptr;
    QPushButton *m_btNew      = nullptr;
    QListWidget *m_lstPal     = nullptr;

    // Time Announce tab
    QCheckBox   *m_ckTmAnn    = nullptr;
    QCheckBox   *m_btTmAnJin  = nullptr;
    QSpinBox    *m_spMin      = nullptr;
    QSpinBox    *m_spSec      = nullptr;
    QSpinBox    *m_spDel      = nullptr;

    // Stream tab
    QCheckBox   *m_ckStream   = nullptr;
    QComboBox   *m_cmbStream  = nullptr;
    QLabel      *m_lblConDisp = nullptr;
    QCheckBox   *m_ckMixByTa  = nullptr;

    // Settings tab
    QSlider     *m_slideVol   = nullptr;
    QLabel      *m_lblVol     = nullptr;
    QLabel      *m_lblVolExp  = nullptr;
    QCheckBox   *m_ckAssVol   = nullptr;
    QCheckBox   *m_ckLoopI    = nullptr;
    QCheckBox   *m_ckTouch    = nullptr;
    QCheckBox   *m_ckAutoRep  = nullptr;
    QCheckBox   *m_ckAutoMix  = nullptr;
    QPushButton *m_btSettings = nullptr;
    QPushButton *m_btAbout    = nullptr;

    // ---- Context menu ----
    QMenu   *m_ctxMenu     = nullptr;
    QAction *m_actPlay     = nullptr;
    QAction *m_actPause    = nullptr;
    QAction *m_actStop     = nullptr;
    QAction *m_actAssign   = nullptr;
    QAction *m_actClear    = nullptr;
    QAction *m_actLoop     = nullptr;
    QAction *m_actVolume   = nullptr;
    QAction *m_actTa       = nullptr;

    // ---- State ----
    int     m_ctxIdx       = -1;    // jingle under context menu
    int     m_volSelIdx    = -1;    // jingle under volume selection
    int     m_vuTrackIdx   = -1;    // jingle being tracked for remaining time
    bool    m_flash        = false;
    int     m_flashCount   = 0;
    bool    m_taJingWaiting = false;

    QString m_appPath;

    SettingsDialog *m_settingsDlg = nullptr;
    AboutDialog    *m_aboutDlg    = nullptr;
};
