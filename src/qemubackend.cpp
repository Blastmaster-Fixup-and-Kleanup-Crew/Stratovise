#include "qemubackend.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

QStringList QemuBackend::commonCandidates() const
{
    QStringList candidates;
    const QString fromPath = QStandardPaths::findExecutable("qemu-system-x86_64");
    if (!fromPath.isEmpty())
        candidates << fromPath;

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
    candidates.removeDuplicates();
    return candidates;
}

QString QemuBackend::executablePath() const
{
    for (const QString &candidate : commonCandidates()) {
        const QFileInfo info(candidate);
        if (info.exists() && info.isFile())
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
        return "QEMU was not found. Install QEMU for Windows or add its bin directory to PATH.";

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
