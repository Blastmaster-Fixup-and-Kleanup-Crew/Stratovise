#pragma once

#include <QHash>
#include <QJsonArray>
#include <QMainWindow>
#include <QString>

class QLabel;
class QProcess;
class QTableWidget;

class MainWindow final : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void createVirtualMachine();
    void startSelectedVirtualMachine();
    void stopSelectedVirtualMachine();
    void deleteSelectedVirtualMachine();
    void showBackendStatus();
    void showAbout();

private:
    void buildMenus();
    void refreshStatus();
    void loadVirtualMachines();
    bool saveVirtualMachines();
    QString configurationFilePath() const;
    QString selectedMachineId() const;
    int rowForMachineId(const QString &id) const;
    void setMachineStatus(const QString &id, const QString &status);

    QLabel *statusLabel_ = nullptr;
    QTableWidget *machineTable_ = nullptr;
    QJsonArray machines_;
    QHash<QString, QProcess *> processes_;
};
