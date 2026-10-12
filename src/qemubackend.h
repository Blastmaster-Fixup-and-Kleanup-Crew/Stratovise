#pragma once

#include <QString>
#include <QStringList>

class QemuBackend
{
public:
    QString executablePath() const;
    bool isAvailable() const;
    QString versionText() const;
    QStringList commonCandidates() const;
};
