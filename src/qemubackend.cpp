#include "qemubackend.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

QStringList QemuBackend::commonCandidates() const
{
    QStringList candidates;
    const QStringList executableNames = {
#ifdef Q_OS_WIN
        "qemu-system-x86_64.exe",
        "qemu-system-x86_64"
#else
        "qemu-system-x86_64",
        "qemu-system-x86_64.exe"
#endif
    };
    for (const QString &name : executableNames) {
        const QString fromPath = QStandardPaths::findExecutable(name);
        if (!fromPath.isEmpty())
            candidates << fromPath;
    }

#ifdef Q_OS_WIN
    const QStringList roots = {
        qEnvironmentVariable("ProgramFiles"),
        qEnvironmentVariable("ProgramW6432"),
        qEnvironmentVariable("LOCALAPPDATA")
    };
    for (const QString &root : roots) {
        if (root.isEmpty())
            continue;
        candidates << QDir(root).filePath("qemu/qemu-system-x86_64.exe");
        candidates << QDir(root).filePath("QEMU/qemu-system-x86_64.exe");
        candidates << QDir(root).filePath("qemu/bin/qemu-system-x86_64.exe");
    }
#else
    candidates << "/usr/bin/qemu-system-x86_64";
    candidates << "/usr/local/bin/qemu-system-x86_64";
    candidates << "/opt/homebrew/bin/qemu-system-x86_64";
#endif
    candidates.removeDuplicates();
    return candidates;
}

QString QemuBackend::executablePath() const
{
    for (const QString &candidate : commonCandidates()) {
        const QFileInfo info(candidate);
        if (info.exists() && info.isFile() && info.isExecutable())
            return info.absoluteFilePath();
    }
    return {};
}

bool QemuBackend::isAvailable() const
{
    return !executablePath().isEmpty();
}

QString QemuBackend::versionText() const
{
    const QString executable = executablePath();
    if (executable.isEmpty())
        return "QEMU was not found. Install QEMU for your host platform or add qemu-system-x86_64 to PATH.";

    QProcess process;
    process.setProgram(executable);
    process.setArguments({"--version"});
    process.start();
    if (!process.waitForStarted(2000))
        return "QEMU was found, but could not be started: " + process.errorString();
    if (!process.waitForFinished(4000)) {
        process.kill();
        process.waitForFinished(1000);
        return "QEMU did not respond to the version check in time.";
    }

    const QString output = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        return "QEMU version check failed: " +
               QString::fromLocal8Bit(process.readAllStandardError()).trimmed();
    return output.isEmpty() ? "QEMU is available." : output;
}
