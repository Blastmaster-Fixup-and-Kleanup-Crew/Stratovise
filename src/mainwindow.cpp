#include "mainwindow.h"
#include "qemubackend.h"

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QAbstractItemView>
#include <QPushButton>
#include <QTableWidgetItem>
#include <QFont>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QSpinBox>
#include <QStatusBar>
#include <QTableWidget>
#include <QToolBar>
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
    machineTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    machineTable_->setAlternatingRowColors(true);
    layout->addWidget(machineTable_);

    auto *hint = new QLabel("No virtual machines registered. Choose File > New Virtual Machine to begin.", central);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    setCentralWidget(central);
    statusLabel_ = new QLabel(this);
    statusBar()->addPermanentWidget(statusLabel_, 1);
    refreshStatus();
}

void MainWindow::buildMenus()
{
    auto *fileMenu = menuBar()->addMenu("&File");
    auto *newVm = fileMenu->addAction("&New Virtual Machine...");
    newVm->setShortcut(QKeySequence::New);
    connect(newVm, &QAction::triggered, this, &MainWindow::createVirtualMachine);
    fileMenu->addSeparator();
    auto *quit = fileMenu->addAction("E&xit");
    quit->setShortcut(QKeySequence::Quit);
    connect(quit, &QAction::triggered, qApp, &QApplication::quit);

    auto *vmMenu = menuBar()->addMenu("&Virtual Machine");
    auto *start = vmMenu->addAction("&Start");
    start->setEnabled(false);
    start->setToolTip("VM lifecycle controls are added in the next milestone.");
    auto *settings = vmMenu->addAction("&Settings...");
    settings->setEnabled(false);

    auto *viewMenu = menuBar()->addMenu("&View");
    auto *refresh = viewMenu->addAction("&Refresh");
    connect(refresh, &QAction::triggered, this, &MainWindow::refreshStatus);

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
        const QString path = QFileDialog::getOpenFileName(&dialog, "Select boot ISO", {},
                                                          "ISO images (*.iso);;All files (*.*)");
        if (!path.isEmpty())
            iso->setText(path);
    });

    form->addRow("Machine name:", name);
    form->addRow("Guest operating system:", guest);
    form->addRow("Memory:", memory);
    form->addRow("Processors:", cpus);
    form->addRow("Boot ISO (optional):", isoRow);
    layout->addLayout(form);
    layout->addWidget(new QLabel("This first milestone captures configuration only. Persistent VM creation and QEMU launch are not implemented yet.", &dialog));

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted)
        return;
    if (name->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "New Virtual Machine", "Enter a name for the virtual machine.");
        return;
    }

    const int row = machineTable_->rowCount();
    machineTable_->insertRow(row);
    machineTable_->setItem(row, 0, new QTableWidgetItem(name->text().trimmed()));
    machineTable_->setItem(row, 1, new QTableWidgetItem(guest->text().trimmed()));
    machineTable_->setItem(row, 2, new QTableWidgetItem("Not created"));
    machineTable_->setItem(row, 3, new QTableWidgetItem(
        QString("%1 MB RAM, %2 CPU(s)%3").arg(memory->value()).arg(cpus->value())
        .arg(iso->text().isEmpty() ? "" : ", ISO: " + iso->text())));
    statusBar()->showMessage("Draft configuration added to this session; it has not been saved or launched.", 7000);
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
        "Stratovise 0.1.0\nWindows desktop VM manager\n\n"
        "An independent, classic-workstation-inspired interface using Qt 6 and QEMU.\n"
        "This development build is an application shell; VM persistence and execution are not yet implemented.");
}

void MainWindow::refreshStatus()
{
    QemuBackend backend;
    statusLabel_->setText(backend.isAvailable() ? "QEMU detected" : "QEMU not detected");
}
