#pragma once

#include <QMainWindow>

class QLabel;
class QTableWidget;

class MainWindow final : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void createVirtualMachine();
    void showBackendStatus();
    void showAbout();

private:
    void buildMenus();
    void refreshStatus();

    QLabel *statusLabel_ = nullptr;
    QTableWidget *machineTable_ = nullptr;
};
