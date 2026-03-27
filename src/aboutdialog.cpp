#include "aboutdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QGroupBox>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("About Jingle Palette"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setModal(true);
    setFixedSize(560, 240);

    QVBoxLayout *v = new QVBoxLayout(this);

    QGroupBox *box = new QGroupBox(tr("Information"));
    QVBoxLayout *vb = new QVBoxLayout(box);

    // Title + version
    QHBoxLayout *h1 = new QHBoxLayout;
    QLabel *lblTitle = new QLabel("<b>Jingle Palette</b>");
    QLabel *lblVer   = new QLabel(QString("Version %1")
                                      .arg(QApplication::applicationVersion()));
    QLabel *lblDesc  = new QLabel(tr("An instant jingle player designed for radio studios."));
    lblDesc->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    h1->addWidget(lblTitle);
    h1->addWidget(lblVer);
    h1->addStretch();
    h1->addWidget(lblDesc);
    vb->addLayout(h1);

    // Authors
    QHBoxLayout *h2 = new QHBoxLayout;
    h2->addWidget(new QLabel(tr("Created by:")));
    h2->addWidget(new QLabel("Horvárkosi Róbert"));
    h2->addSpacing(20);
    QLabel *lblEmail = new QLabel(
        "<a href='mailto:horvark@gmail.com'>horvark@gmail.com</a>");
    lblEmail->setOpenExternalLinks(true);
    h2->addWidget(new QLabel("Email:"));
    h2->addWidget(lblEmail);
    h2->addStretch();
    vb->addLayout(h2);

    // Thanks
    QHBoxLayout *h3 = new QHBoxLayout;
    h3->addWidget(new QLabel(tr("Special thanks to:")));
    h3->addWidget(new QLabel("Nagy Attila"));
    h3->addStretch();
    QLabel *lblWeb = new QLabel(
        "<a href='http://www.horvark.hu/jinglepalette'>http://www.horvark.hu/jinglepalette</a>");
    lblWeb->setOpenExternalLinks(true);
    h3->addWidget(new QLabel("Web:"));
    h3->addWidget(lblWeb);
    vb->addLayout(h3);

    QFrame *line = new QFrame; line->setFrameShape(QFrame::HLine);
    vb->addWidget(line);

    QLabel *lblLic = new QLabel(tr(
        "This is freeware, with no warranty of any kind.\n"
        "You may use it free of charge for personal or commercial radio use.\n"
        "You may not sell this software."));
    vb->addWidget(lblLic);

    QLabel *lblBass = new QLabel(tr("This program uses the Bass audio library, © 1999-2004 Ian Luck. All rights reserved."));
    lblBass->setStyleSheet("font-size:9px; color:#666;");
    vb->addWidget(lblBass);

    v->addWidget(box);

    QPushButton *ok = new QPushButton(tr("OK"));
    ok->setDefault(true);
    connect(ok, &QPushButton::clicked, this, &QDialog::accept);
    QHBoxLayout *hOk = new QHBoxLayout;
    hOk->addStretch();
    hOk->addWidget(ok);
    v->addLayout(hOk);
}
