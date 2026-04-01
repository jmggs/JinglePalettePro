#pragma once
#include <QMainWindow>
#include "remotecontrolserver.h"
#include <QLabel>
#include <QTimer>
#include <QTabWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QListWidget>
#include <QLineEdit>
#include <QSlider>
#include <QMenu>
#include <QAction>
#include <QSpinBox>
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

    RemoteControlServer *m_remoteServer = nullptr;
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void loadPalette(const QString &name);
    void refreshPaletteList();

protected:
    void closeEvent(QCloseEvent *) override;
    void keyPressEvent(QKeyEvent *) override;

private slots:
    void onJingleClicked(int idx);
    void onJingleRightClicked(int idx);
    void onTimer1();
    void onTimer2();
    void onTimer3();
    void onTimer5();
    void onAssignToggled(bool on);
    void onSavePalette();
    void onNewPalette();
    void onPaletteSelected(const QString &name);
    void onTimeAnnounceToggled(bool on);
    void onSelectTaJingle(bool on);
    void onMasterVolumeChanged(int val);
    void onMenuPlay();
    void onMenuPause();
    void onMenuStop();
    void onMenuAssign();
    void onMenuClear();
    void onMenuLoop();
    void onMenuVolume();
    void onMenuTimeAnnounce();
    void onSettingsClicked();
    void onAboutClicked();
    void onJingleFinished(int idx);
    void onVuLevels(float left, float right);
    void onPrevPalette();
    void onNextPalette();
    void onBtnAutoRepeat();
    void onBtnPause();
    void onBtnAutoMix();
    void onBtnStop();

private:
    void setupUi();
    void setupContextMenu();
    void applyDarkTheme();
    void applySettings();
    void saveWindowState();
    void restoreWindowState();
    void assignJingle(int idx);
    void playJingle(int idx);
    void stopAll();
    void updateDisplayTime();
    void updateRemainingTime();
    void updatePaletteDisplay();
    QString formatJingleName(const QString &path, int maxLen = 60) const;

    // Managers
    AudioEngine    *m_audio = nullptr;
    PaletteManager *m_pal   = nullptr;
    SettingsManager*m_set   = nullptr;
    LanguageManager*m_lang  = nullptr;

    // Timers
    QTimer *m_timer1 = nullptr;
    QTimer *m_timer2 = nullptr;
    QTimer *m_timer3 = nullptr;
    QTimer *m_timer5 = nullptr;

    // Top bar
    QLabel      *m_lblPalName = nullptr;
    QPushButton *m_btnPrev    = nullptr;
    QPushButton *m_btnNext    = nullptr;
    QLabel      *m_lblRemTime = nullptr;
    QLabel      *m_lblTime    = nullptr;
    QLabel      *m_lblDate    = nullptr;
    QLabel      *m_lblErrIcon = nullptr;
    QLabel      *m_lblTaIcon  = nullptr;

    // Jingle grid
    std::array<JingleButton*, JINGLE_COUNT> m_jingBtns = {};

    // VU meters
    VuMeter *m_vuL    = nullptr;
    VuMeter *m_vuR    = nullptr;
    QLabel  *m_lblVuL = nullptr;
    QLabel  *m_lblVuR = nullptr;

    // Tab panel
    QTabWidget  *m_tabWidget  = nullptr;

    // Tab: Palettes
    QCheckBox   *m_ckAssign   = nullptr;
    QLineEdit   *m_txtAssign  = nullptr;
    QPushButton *m_btSave     = nullptr;
    QPushButton *m_btNew      = nullptr;
    QLineEdit   *m_txtSave    = nullptr;
    QListWidget *m_lstPal     = nullptr;

    // Tab: Time Announce
    QCheckBox   *m_ckTmAnn    = nullptr;
    QCheckBox   *m_btTmAnJin  = nullptr;
    QSpinBox    *m_spMin      = nullptr;
    QSpinBox    *m_spSec      = nullptr;
    QSpinBox    *m_spDel      = nullptr;

    // Tab: Settings
    QSlider     *m_slideVol   = nullptr;
    QLabel      *m_lblVolVal  = nullptr;
    QPushButton *m_btSettings = nullptr;
    QPushButton *m_btAbout    = nullptr;

    // Big control buttons (bottom right)
    QPushButton *m_btnAutoRepeat  = nullptr;
    QPushButton *m_btnPause       = nullptr;
    QPushButton *m_btnAutoMix     = nullptr;
    QPushButton *m_btnStop        = nullptr;
    QWidget     *m_ctrlTopSpacer  = nullptr;

    // Context menu
    QMenu   *m_ctxMenu   = nullptr;
    QAction *m_actPlay   = nullptr;
    QAction *m_actPause  = nullptr;
    QAction *m_actStop   = nullptr;
    QAction *m_actAssign = nullptr;
    QAction *m_actClear  = nullptr;
    QAction *m_actLoop   = nullptr;
    QAction *m_actVolume = nullptr;
    QAction *m_actTa     = nullptr;

    // State
    int     m_ctxIdx        = -1;
    int     m_volSelIdx     = -1;
    int     m_vuTrackIdx    = -1;
    bool    m_flash         = false;
    int     m_flashCount    = 0;
    bool    m_taJingWaiting = false;
    QString m_appPath;

    SettingsDialog *m_settingsDlg = nullptr;
    AboutDialog    *m_aboutDlg    = nullptr;
};
