#pragma once
#include <QDialog>
#include <QTabWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QLineEdit>
#include <QFrame>
#include <QEvent>
#include <QMouseEvent>
#include <QColor>
#include <QNetworkAccessManager>

class SettingsManager;
class LanguageManager;
class AudioEngine;

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(SettingsManager *set, LanguageManager *lang,
                            AudioEngine *audio, QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *obj, QEvent *e) override;

private slots:
    void onVolumeChanged(int val);
    void onSelectTaSound();
    void onDeviceChanged(int idx);
    void onColorSelect();
    void onColorReset();
    void accept() override;

private:
    void setupUi();
    void loadValues();

    SettingsManager  *m_set;
    LanguageManager  *m_lang;
    AudioEngine      *m_audio;
    QNetworkAccessManager *m_net = nullptr;

    // Display tab
    QComboBox  *m_cmbTime    = nullptr;
    QComboBox  *m_cmbDate    = nullptr;
    QSpinBox   *m_spRemWarn  = nullptr;
    QFrame     *m_colSel     = nullptr;
    QSpinBox   *m_spAmix     = nullptr;

    // Audio tab
    QSlider    *m_slideVol   = nullptr;
    QLabel     *m_lblVol     = nullptr;
    QPushButton*m_btSound    = nullptr;
    QComboBox  *m_cmbDev     = nullptr;

    // Others tab
    QComboBox  *m_cmbThread  = nullptr;
    QComboBox  *m_cmbLang    = nullptr;
    QCheckBox  *m_ckErrLog   = nullptr;
    QCheckBox  *m_ckOnTop    = nullptr;

    QColor      m_remColor;

    // Remote HTTP tab
    QSpinBox   *m_spRemotePort = nullptr;
    QCheckBox  *m_ckRemoteEnable = nullptr;
    int m_remotePort = 8000;
    void onRemotePortChanged(int port);
    void onRemoteEnableToggled(bool on);
};
