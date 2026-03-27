#pragma once
#include <QString>
#include <QStringList>
#include "globals.h"

class PaletteManager {
public:
    explicit PaletteManager(const QString &palPath);

    // Returns list of saved palette names
    QStringList paletteNames() const;

    // Loads jingle data (paths/volume/loop) from a named section
    void loadPalette(const QString &name, JingleData jings[JINGLE_COUNT]);

    // Saves current jingle data into a named section
    void savePalette(const QString &name, const JingleData jings[JINGLE_COUNT]);

    // Deletes a palette section
    void deletePalette(const QString &name);

    QString palPath() const { return m_palPath; }

private:
    QString m_palPath;
};
