#include "aboutdialog.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("About Jingle Palette Pro"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setModal(true);
    setFixedSize(620, 340);

    QVBoxLayout *v = new QVBoxLayout(this);

    QLabel *content = new QLabel(
        "<b>Jingle Palette Pro v0.2</b><br><br>"
        "This is a full cross-platform port of the original Jingle Palette based on original program "
        "written by H. Árkosi Róbert (nagyrobi)<br><br>"
        "<a href='https://github.com/jmggs/JinglePalettepro'>https://github.com/jmggs/JinglePalettepro</a><br><br>"
        "<b>Keyboard Shortcuts:</b><br>"
        "1-5 -> Jingles 1-5<br>"
        "Q W E R T - Jingles 6-10<br>"
        "A S D F G - Jingles 11-15<br>"
        "Z X C V B - Jingles 16-20<br>"
        "Space - Pause<br><br>"
        "<b>Http API:</b><br>"
        "http://&lt;IP&gt;:8000/<br>"
        "/01 to /30 - Jingles<br>"
        "/pause /stop /automix /autorepeat");
    content->setWordWrap(true);
    content->setOpenExternalLinks(true);
    content->setTextInteractionFlags(Qt::TextBrowserInteraction);
    v->addWidget(content);

    QPushButton *ok = new QPushButton(tr("OK"));
    ok->setDefault(true);
    connect(ok, &QPushButton::clicked, this, &QDialog::accept);
    QHBoxLayout *hOk = new QHBoxLayout;
    hOk->addStretch();
    hOk->addWidget(ok);
    v->addLayout(hOk);
}
