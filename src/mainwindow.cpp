#include "mainwindow.h"
#include "qemubackend.h"

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFont>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolBar>
#include <QUuid>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("Stratovise Virtual Machine Manager");
    resize(900, 570);
    buildMenus();

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    auto *heading = new QLabel("Virtual Machines", central);
    QFont headingFont = heading->font();
    headingFont.setBold(true);
    headingFont.setPointSize(headingFont.pointSize() + 2);
    heading->setFont(headingFont);
    layout->addWidget(heading);

    machineTable_ = new QTableWidget(0, 4, central);
    machineTable_->setHorizontalHeaderLabels({"Name", "Guest OS", "Status", "Configuration"});
    machineTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    machineTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    machineTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    machineTable_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    machineTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    machineTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    machineTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    machineTable_->setAlternatingRowColors(true);
    layout->addWidget(machineTable_);

    auto *hint = new QLabel(
        "VM definitions are stored in your user configuration directory. Select a VM to start, stop, or remove it.",
        central);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    setCentralWidget(central);
    statusLabel_ = new QLabel(this);
    statusBar()->addPermanentWidget(statusLabel_, 1);
    loadVirtualMachines();
    refreshStatus();
}

MainWindow::~MainWindow()
{
    const auto running = processes_.values();
    for (QProcess *process : running) {
        if (process && process->state() != QProcess::NotRunning) {
            process->terminate();
            if (!process->waitForFinished(2000)) {
                process->kill();
                process->waitForFinished(1000);
            }
        }
    }
}

void MainWindow::buildMenus()
{
    auto *fileMenu = menuBar()->addMenu("&File");
    auto *newVm = fileMenu->addAction("&New Virtual Machine...");
    newVm->setShortcut(QKeySequence::New);
    connect(newVm, &QAction::triggered, this, &MainWindow::createVirtualMachine);
    auto *removeVm = fileMenu->addAction("&Remove Selected Virtual Machine");
    connect(removeVm, &QAction::triggered, this, &MainWindow::deleteSelectedVirtualMachine);
    fileMenu->addSeparator();
    auto *quit = fileMenu->addAction("E&xit");
    quit->setShortcut(QKeySequence::Quit);
    connect(quit, &QAction::triggered, qApp, &QApplication::quit);

    auto *vmMenu = menuBar()->addMenu("&Virtual Machine");
    auto *start = vmMenu->addAction("&Start");
    start->setShortcut(Qt::Key_F5);
    connect(start, &QAction::triggered, this, &MainWindow::startSelectedVirtualMachine);
    auto *stop = vmMenu->addAction("S&top");
    stop->setShortcut(Qt::Key_F6);
    connect(stop, &QAction::triggered, this, &MainWindow::stopSelectedVirtualMachine);
    vmMenu->addSeparator();
    auto *settings = vmMenu->addAction("&Settings...");
    settings->setEnabled(false);
    settings->setToolTip("Editable hardware settings are planned for a later milestone.");

    auto *viewMenu = menuBar()->addMenu("&View");
    auto *refresh = viewMenu->addAction("&Refresh");
    connect(refresh, &QAction::triggered, this, [this]() {
        loadVirtualMachines();
        refreshStatus();
    });

    auto *helpMenu = menuBar()->addMenu("&Help");
    auto *backend = helpMenu->addAction("QEMU &Backend Status...");
    connect(backend, &QAction::triggered, this, &MainWindow::showBackendStatus);
    helpMenu->addSeparator();
    auto *about = helpMenu->addAction("&About Stratovise");
    connect(about, &QAction::triggered, this, &MainWindow::showAbout);

    auto *toolbar = addToolBar("Standard");
    toolbar->setObjectName("standardToolBar");
    toolbar->setMovable(false);
    auto *toolbarNew = toolbar->addAction("New VM");
    connect(toolbarNew, &QAction::triggered, this, &MainWindow::createVirtualMachine);
    auto *toolbarStart = toolbar->addAction("Start");
    connect(toolbarStart, &QAction::triggered, this, &MainWindow::startSelectedVirtualMachine);
    auto *toolbarStop = toolbar->addAction("Stop");
    connect(toolbarStop, &QAction::triggered, this, &MainWindow::stopSelectedVirtualMachine);
    auto *toolbarStatus = toolbar->addAction("Backend Status");
    connect(toolbarStatus, &QAction::triggered, this, &MainWindow::showBackendStatus);
}

void MainWindow::createVirtualMachine()
{
    QDialog dialog(this);
    dialog.setWindowTitle("New Virtual Machine");
    dialog.setModal(true);
    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout();

    auto *name = new QLineEdit("New Virtual Machine", &dialog);
    auto *guest = new QLineEdit("Other", &dialog);
    guest->setPlaceholderText("e.g. Windows 11, Windows XP, Linux");
    auto *memory = new QSpinBox(&dialog);
    memory->setRange(512, 1048576);
    memory->setSingleStep(512);
    memory->setValue(4096);
    memory->setSuffix(" MB");
    auto *cpus = new QSpinBox(&dialog);
    cpus->setRange(1, 64);
    cpus->setValue(2);
    auto *iso = new QLineEdit(&dialog);
    auto *isoRow = new QWidget(&dialog);
    auto *isoLayout = new QHBoxLayout(isoRow);
    isoLayout->setContentsMargins(0, 0, 0, 0);
    isoLayout->addWidget(iso);
    auto *browseButton = new QPushButton("Browse...", isoRow);
    isoLayout->addWidget(browseButton);
    connect(browseButton, &QPushButton::clicked, &dialog, [iso, &dialog]() {
        const QString path = QFileDialog::getOpenFileName(
            &dialog, "Select boot ISO", {}, "ISO images (*.iso);;All files (*.*)");
        if (!path.isEmpty())
            iso->setText(path);
    });

    form->addRow("Machine name:", name);
    form->addRow("Guest operating system:", guest);
    form->addRow("Memory:", memory);
    form->addRow("Processors:", cpus);
    form->addRow("Boot ISO:", isoRow);
    layout->addLayout(form);
    layout->addWidget(new QLabel(
        "A bootable ISO is required to start this early build. Virtual disks and editable hardware settings are not implemented yet.",
        &dialog));

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString machineName = name->text().trimmed();
    if (machineName.isEmpty()) {
        QMessageBox::warning(this, "New Virtual Machine", "Enter a name for the virtual machine.");
        return;
    }
    for (const QJsonValue &value : machines_) {
        if (value.toObject().value("name").toString().compare(machineName, Qt::CaseInsensitive) == 0) {
            QMessageBox::warning(this, "New Virtual Machine", "A virtual machine with that name already exists.");
            return;
        }
    }

    QJsonObject machine;
    machine.insert("id", QUuid::createUuid().toString(QUuid::WithoutBraces));
    machine.insert("name", machineName);
    machine.insert("guestOs", guest->text().trimmed().isEmpty() ? "Other" : guest->text().trimmed());
    machine.insert("memoryMb", memory->value());
    machine.insert("cpus", cpus->value());
    const QString isoText = iso->text().trimmed();
    machine.insert("isoPath", isoText.isEmpty() ? QString() : QDir::cleanPath(isoText));
    machines_.append(machine);
    if (!saveVirtualMachines()) {
        machines_.removeLast();
        return;
    }

    loadVirtualMachines();
    statusBar()->showMessage("Virtual machine definition saved.", 5000);
}

void MainWindow::startSelectedVirtualMachine()
{
    const QString id = selectedMachineId();
    if (id.isEmpty()) {
        QMessageBox::information(this, "Start Virtual Machine", "Select a virtual machine first.");
        return;
    }
    if (processes_.contains(id) && processes_.value(id)
        && processes_.value(id)->state() != QProcess::NotRunning) {
        statusBar()->showMessage("That virtual machine is already running.", 4000);
        return;
    }

    QJsonObject machine;
    for (const QJsonValue &value : machines_) {
        if (value.toObject().value("id").toString() == id) {
            machine = value.toObject();
            break;
        }
    }
    if (machine.isEmpty())
        return;

    const QString isoPath = machine.value("isoPath").toString();
    if (isoPath.isEmpty() || !QFileInfo::exists(isoPath)) {
        QMessageBox::warning(this, "Start Virtual Machine",
            "Choose an existing bootable ISO when creating the VM. Virtual disk boot is not supported in this milestone.");
        return;
    }

    QemuBackend backend;
    const QString executable = backend.executablePath();
    if (executable.isEmpty()) {
        QMessageBox::critical(this, "Start Virtual Machine",
            "QEMU was not found. Install QEMU and ensure qemu-system-x86_64 is on PATH.");
        return;
    }

    auto *process = new QProcess(this);
    QStringList arguments{
        "-name", machine.value("name").toString(),
        "-m", QString::number(machine.value("memoryMb").toInt(4096)),
        "-smp", QString::number(machine.value("cpus").toInt(2)),
        "-cdrom", isoPath,
        "-boot", "order=d",
        "-display", "default"
    };
    process->setProgram(executable);
    process->setArguments(arguments);
    process->setProcessChannelMode(QProcess::MergedChannels);
    processes_.insert(id, process);
    connect(process, &QProcess::started, this, [this, id]() {
        setMachineStatus(id, "Running");
        statusBar()->showMessage("QEMU started.", 4000);
    });
    connect(process, &QProcess::readyRead, this, [this, process, id]() {
        const QString output = QString::fromLocal8Bit(process->readAll()).trimmed();
        if (!output.isEmpty())
            statusBar()->showMessage("QEMU (" + id.left(8) + "): " + output.left(180), 8000);
    });
    connect(process, &QProcess::errorOccurred, this, [this, process, id](QProcess::ProcessError) {
        if (process->state() == QProcess::NotRunning) {
            setMachineStatus(id, "Stopped");
            statusBar()->showMessage("QEMU launch failed: " + process->errorString(), 8000);
        }
    });
    connect(process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, [this, id](int, QProcess::ExitStatus) {
        setMachineStatus(id, "Stopped");
    });
    setMachineStatus(id, "Starting");
    process->start();
}

void MainWindow::stopSelectedVirtualMachine()
{
    const QString id = selectedMachineId();
    if (id.isEmpty()) {
        QMessageBox::information(this, "Stop Virtual Machine", "Select a virtual machine first.");
        return;
    }
    QProcess *process = processes_.value(id, nullptr);
    if (!process || process->state() == QProcess::NotRunning) {
        setMachineStatus(id, "Stopped");
        return;
    }
    process->terminate();
    if (!process->waitForFinished(2500)) {
        process->kill();
        process->waitForFinished(1000);
    }
    setMachineStatus(id, "Stopped");
}

void MainWindow::deleteSelectedVirtualMachine()
{
    const QString id = selectedMachineId();
    if (id.isEmpty()) {
        QMessageBox::information(this, "Remove Virtual Machine", "Select a virtual machine first.");
        return;
    }
    QProcess *process = processes_.value(id, nullptr);
    if (process && process->state() != QProcess::NotRunning) {
        QMessageBox::warning(this, "Remove Virtual Machine", "Stop the VM before removing its definition.");
        return;
    }
    if (QMessageBox::question(this, "Remove Virtual Machine",
            "Remove the selected VM definition? This does not delete any ISO or disk files.") != QMessageBox::Yes)
        return;

    for (qsizetype i = 0; i < machines_.size(); ++i) {
        if (machines_.at(i).toObject().value("id").toString() == id) {
            machines_.removeAt(i);
            break;
        }
    }
    if (process) {
        processes_.remove(id);
        process->deleteLater();
    }
    if (saveVirtualMachines())
        loadVirtualMachines();
}

void MainWindow::showBackendStatus()
{
    QemuBackend backend;
    const QString path = backend.executablePath();
    QMessageBox::information(this, "QEMU Backend Status",
        backend.versionText() + (path.isEmpty() ? QString() : "\n\nExecutable: " + path));
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, "About Stratovise",
        "Stratovise 0.2.0\nCross-platform desktop VM manager\n\n"
        "An independent, classic-workstation-inspired interface using Qt 6 and QEMU. "
        "VM definitions are saved locally; VM execution currently boots from ISO only.");
}

void MainWindow::refreshStatus()
{
    QemuBackend backend;
    statusLabel_->setText(backend.isAvailable() ? "QEMU detected" : "QEMU not detected");
}

QString MainWindow::configurationFilePath() const
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation))
        .filePath("machines.json");
}

void MainWindow::loadVirtualMachines()
{
    QFile file(configurationFilePath());
    if (file.exists()) {
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(this, "Load Virtual Machines",
                "Could not read saved VM definitions: " + file.errorString());
            return;
        }
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isArray()) {
            QMessageBox::warning(this, "Load Virtual Machines",
                "The saved VM definition file is invalid. It was left untouched at:\n" + file.fileName());
            return;
        }
        machines_ = document.array();
    } else {
        machines_ = QJsonArray{};
    }

    machineTable_->setRowCount(0);
    for (const QJsonValue &value : machines_) {
        const QJsonObject machine = value.toObject();
        const QString id = machine.value("id").toString();
        if (id.isEmpty())
            continue;
        const int row = machineTable_->rowCount();
        machineTable_->insertRow(row);
        auto *nameItem = new QTableWidgetItem(machine.value("name").toString("Unnamed VM"));
        nameItem->setData(Qt::UserRole, id);
        machineTable_->setItem(row, 0, nameItem);
        machineTable_->setItem(row, 1, new QTableWidgetItem(machine.value("guestOs").toString("Other")));
        const QProcess *process = processes_.value(id, nullptr);
        const QString state = process && process->state() != QProcess::NotRunning ? "Running" : "Stopped";
        machineTable_->setItem(row, 2, new QTableWidgetItem(state));
        const QString iso = machine.value("isoPath").toString();
        machineTable_->setItem(row, 3, new QTableWidgetItem(
            QString("%1 MB RAM, %2 CPU(s)%3")
                .arg(machine.value("memoryMb").toInt(4096))
                .arg(machine.value("cpus").toInt(2))
                .arg(iso.isEmpty() ? "" : ", ISO: " + iso)));
    }
}

bool MainWindow::saveVirtualMachines()
{
    const QString path = configurationFilePath();
    const QFileInfo info(path);
    if (!QDir().mkpath(info.absolutePath())) {
        QMessageBox::critical(this, "Save Virtual Machines",
            "Could not create the configuration directory:\n" + info.absolutePath());
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, "Save Virtual Machines",
            "Could not save VM definitions: " + file.errorString());
        return false;
    }
    const QByteArray data = QJsonDocument(machines_).toJson(QJsonDocument::Indented);
    if (file.write(data) != data.size()) {
        QMessageBox::critical(this, "Save Virtual Machines",
            "The VM definition file could not be written completely.");
        return false;
    }
    return true;
}

QString MainWindow::selectedMachineId() const
{
    const int row = machineTable_->currentRow();
    if (row < 0 || !machineTable_->item(row, 0))
        return {};
    return machineTable_->item(row, 0)->data(Qt::UserRole).toString();
}

int MainWindow::rowForMachineId(const QString &id) const
{
    for (int row = 0; row < machineTable_->rowCount(); ++row) {
        if (machineTable_->item(row, 0)
            && machineTable_->item(row, 0)->data(Qt::UserRole).toString() == id)
            return row;
    }
    return -1;
}

void MainWindow::setMachineStatus(const QString &id, const QString &status)
{
    const int row = rowForMachineId(id);
    if (row >= 0 && machineTable_->item(row, 2))
        machineTable_->item(row, 2)->setText(status);
}
