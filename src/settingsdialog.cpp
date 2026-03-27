#include "settingsdialog.h"
#include "settingsmanager.h"
#include "languagemanager.h"
#include "audioengine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QColorDialog>
#include <QPainter>
#include <QLabel>

SettingsDialog::SettingsDialog(SettingsManager *set, LanguageManager *lang,
                               AudioEngine *audio, QWidget *parent)
    : QDialog(parent), m_set(set), m_lang(lang), m_audio(audio)
{
    setWindowTitle(tr("Configuration Settings"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setModal(true);
    setupUi();
    loadValues();
}

void SettingsDialog::setupUi()
{
    QVBoxLayout *vMain = new QVBoxLayout(this);

    QTabWidget *tabs = new QTabWidget;

    // ── Display tab ──────────────────────────────────────────────────────
    {
        QWidget *w = new QWidget;
        QFormLayout *f = new QFormLayout(w);

        m_cmbTime = new QComboBox;
        m_cmbTime->addItems({"HH:mm:ss", "hh:mm:ss AP", "HH:mm", "h:mm AP"});
        m_cmbTime->setEditable(true);

        m_cmbDate = new QComboBox;
        m_cmbDate->addItems({"yyyy MMMM d", "dd/MM/yyyy", "MM/dd/yyyy",
                             "d MMMM yyyy", "dddd d MMMM yyyy"});
        m_cmbDate->setEditable(true);

        m_spRemWarn = new QSpinBox;
        m_spRemWarn->setRange(1, 59);

        m_colSel = new QFrame;
        m_colSel->setFixedSize(60, 22);
        m_colSel->setFrameStyle(QFrame::Box);
        m_colSel->setCursor(Qt::PointingHandCursor);
        m_colSel->setToolTip(tr("Click to select color, Right-click to reset"));
        m_colSel->installEventFilter(this);

        m_spAmix = new QSpinBox;
        m_spAmix->setRange(100, 3000);
        m_spAmix->setSuffix(" ms");
        m_spAmix->setSingleStep(100);

        f->addRow(tr("Time format:"),         m_cmbTime);
        f->addRow(tr("Date format:"),         m_cmbDate);
        f->addRow(tr("Remaining warning (s):"),m_spRemWarn);
        f->addRow(tr("Warning color:"),       m_colSel);
        f->addRow(tr("Auto Mix duration:"),   m_spAmix);

        tabs->addTab(w, tr("Display"));
    }

    // ── Audio tab ─────────────────────────────────────────────────────────
    {
        QWidget *w = new QWidget;
        QVBoxLayout *v = new QVBoxLayout(w);
        QFormLayout *f = new QFormLayout;

        m_slideVol = new QSlider(Qt::Horizontal);
        m_slideVol->setRange(0, 100);
        m_lblVol   = new QLabel("0 dB");
        connect(m_slideVol, &QSlider::valueChanged, this, &SettingsDialog::onVolumeChanged);

        QHBoxLayout *hVol = new QHBoxLayout;
        hVol->addWidget(m_slideVol, 1);
        hVol->addWidget(m_lblVol);

        m_btSound = new QPushButton(tr("(none) — click to assign time announce sound"));
        m_btSound->setToolTip(tr("Assigned time announce sound. Click to assign another."));
        connect(m_btSound, &QPushButton::clicked, this, &SettingsDialog::onSelectTaSound);

        m_cmbDev = new QComboBox;
        for (const QString &d : AudioEngine::availableDevices())
            m_cmbDev->addItem(d);
        connect(m_cmbDev, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &SettingsDialog::onDeviceChanged);

        f->addRow(tr("Main volume:"),   hVol);
        f->addRow(tr("Time announce sound:"), m_btSound);
        f->addRow(tr("Audio device:"),  m_cmbDev);

        v->addLayout(f);
        v->addStretch();
        tabs->addTab(w, tr("Audio"));
    }

    // ── Others tab ────────────────────────────────────────────────────────
    {
        QWidget *w = new QWidget;
        QFormLayout *f = new QFormLayout(w);

        m_cmbThread = new QComboBox;
        m_cmbThread->addItems({tr("Normal"), tr("Above Normal"), tr("High"), tr("Real Time")});

        m_cmbLang = new QComboBox;
        // Languages are populated in loadValues from LanguageManager

        m_ckErrLog = new QCheckBox(tr("Save all errors to \"Error.log\" file"));
        m_ckOnTop  = new QCheckBox(tr("Always On Top of all the other windows"));

        f->addRow(tr("Process priority:"), m_cmbThread);
        f->addRow(tr("Interface language:"), m_cmbLang);
        f->addRow("", m_ckErrLog);
        f->addRow("", m_ckOnTop);

        tabs->addTab(w, tr("Others"));
    }

    vMain->addWidget(tabs);

    QDialogButtonBox *bb = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(bb, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    vMain->addWidget(bb);

    resize(440, 320);
}

void SettingsDialog::loadValues()
{
    // Display
    m_cmbTime->setCurrentText(m_set->timeFormat);
    m_cmbDate->setCurrentText(m_set->dateFormat);
    m_spRemWarn->setValue(m_set->remainWarn);
    m_remColor = m_set->remainColor;
    QPalette p = m_colSel->palette();
    p.setColor(QPalette::Window, m_remColor);
    m_colSel->setAutoFillBackground(true);
    m_colSel->setPalette(p);
    m_spAmix->setValue(m_set->autoMixTime);

    // Audio
    m_slideVol->setValue(m_set->volume);
    m_lblVol->setText(QString::number(m_set->volume - 100) + " dB");
    if (!m_set->timeAnnouncer.isEmpty())
        m_btSound->setText(QFileInfo(m_set->timeAnnouncer).fileName());
    int devIdx = qMax(0, m_set->device);
    if (devIdx < m_cmbDev->count()) m_cmbDev->setCurrentIndex(devIdx);

    // Others
    m_cmbThread->setCurrentIndex(qBound(0, m_set->threadPriority, 3));
    // Languages
    QStringList langs = m_lang->availableLanguages();
    if (langs.isEmpty()) langs << "English";
    m_cmbLang->clear();
    m_cmbLang->addItems(langs);
    int langIdx = langs.indexOf(m_set->language);
    if (langIdx >= 0) m_cmbLang->setCurrentIndex(langIdx);
    m_ckErrLog->setChecked(m_set->errorLog);
    m_ckOnTop->setChecked(m_set->alwaysOnTop);
}

void SettingsDialog::onVolumeChanged(int val)
{
    m_lblVol->setText(QString::number(val - 100) + " dB");
}

void SettingsDialog::onSelectTaSound()
{
    QString path = QFileDialog::getOpenFileName(this,
        tr("Select Time Announce Sound"), m_set->timeAnnouncer,
        tr("Audio Files (*.wav *.mp3 *.mp2 *.ogg)"));
    if (!path.isEmpty()) {
        m_set->timeAnnouncer = path;
        m_btSound->setText(QFileInfo(path).fileName());
    }
}

void SettingsDialog::onDeviceChanged(int idx)
{
    m_set->device = idx;
}

void SettingsDialog::onColorSelect()
{
    QColor c = QColorDialog::getColor(m_remColor, this, tr("Select Warning Color"));
    if (c.isValid()) {
        m_remColor = c;
        QPalette p = m_colSel->palette();
        p.setColor(QPalette::Window, c);
        m_colSel->setPalette(p);
    }
}

void SettingsDialog::onColorReset()
{
    m_remColor = QColor(0, 0, 192);
    QPalette p = m_colSel->palette();
    p.setColor(QPalette::Window, m_remColor);
    m_colSel->setPalette(p);
}

bool SettingsDialog::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == m_colSel) {
        if (e->type() == QEvent::MouseButtonPress) {
            auto *me = static_cast<QMouseEvent*>(e);
            if (me->button() == Qt::LeftButton)  onColorSelect();
            if (me->button() == Qt::RightButton) onColorReset();
            return true;
        }
    }
    return QDialog::eventFilter(obj, e);
}

void SettingsDialog::accept()
{
    m_set->timeFormat    = m_cmbTime->currentText();
    m_set->dateFormat    = m_cmbDate->currentText();
    m_set->remainWarn    = m_spRemWarn->value();
    m_set->remainColor   = m_remColor;
    m_set->autoMixTime   = m_spAmix->value();
    m_set->volume        = m_slideVol->value();
    m_set->device        = m_cmbDev->currentIndex();
    m_set->threadPriority= m_cmbThread->currentIndex();
    m_set->language      = m_cmbLang->currentText();
    m_set->errorLog      = m_ckErrLog->isChecked();
    m_set->alwaysOnTop   = m_ckOnTop->isChecked();
    QDialog::accept();
}
