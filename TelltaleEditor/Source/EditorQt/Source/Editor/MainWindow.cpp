#include <Editor/MainWindow.hpp>
#include <Editor/Dialogs/SelectGameDialog.hpp>
#include <Editor/Window/ArchiveBrowser.hpp>
#include <Editor/Window/ResourceLocations.hpp>

#include <QStyle>
#include <QMenuBar>


MainWindow::MainWindow(Application& app, QWidget* parent) : QMainWindow(parent), _Application(app)
{

	setWindowTitle("Telltale Editor " TTE_VERSION " (No Active Game)");
	resize(1280, 720);
	setWindowIcon(QIcon(":/icons/LogoSquare.png"));

	_SetupMenus();
	_SetupToolBar();
	_SetupCentralWidget();
}

MainWindow::~MainWindow() {}

void MainWindow::_SetupMenus()
{
	QMenuBar* mainMenu = this->menuBar();
    auto* gameBar = mainMenu->addMenu(tr("main.toolbar.game"));
    auto* select = gameBar->addAction(tr("main.toolbar.game.change"));

    QObject::connect(select, &QAction::triggered, this, [this]() 
	{
        SelectGameDialog dlg(_Application, this);
        if (dlg.exec() != QDialog::Accepted)
            return;

        const GameSnapshot& snapshot = dlg.GetSnapshot();
		_Application.Switch(snapshot);

        setWindowTitle(QString("Telltale Editor " TTE_VERSION " %1 (%2, %3)")
            .arg(QString::fromStdString(snapshot.ID), QString::fromStdString(snapshot.Platform), QString::fromStdString(snapshot.Vendor.empty() ? "Default" : snapshot.Vendor)));
    });

    mainMenu->addMenu(tr("main.toolbar.file"));
    mainMenu->addMenu(tr("main.toolbar.editor"));

	// ======================== WINDOW SUBMENU =======================

	auto *windowMenu = mainMenu->addMenu(tr("main.toolbar.window"));

	// ARCHIVE BROWSER
	auto* arcBrowser = windowMenu->addAction(tr("main.toolbar.window.arcbrowser"));
	QObject::connect(arcBrowser, &QAction::triggered, this, [this]()
    {
            auto* w = new ArchiveBrowserWindow(_Application, this);
            w->setAttribute(Qt::WA_DeleteOnClose);
            w->show();
    });

    // RESOURCE LOCATIONS
    auto* resourceLocs = windowMenu->addAction(tr("main.toolbar.window.resourcelocs"));
    resourceLocs->setEnabled(false);
    QObject::connect(resourceLocs, &QAction::triggered, this, [this]()
    {
        ResourceLocationsWindow w(_Application, this);
		w.exec();
    });

	// ===============================================================

    mainMenu->addMenu(tr("main.toolbar.scripts"));
    mainMenu->addMenu(tr("main.toolbar.scene"));
    mainMenu->addMenu(tr("main.toolbar.props"));
    mainMenu->addMenu(tr("main.toolbar.chore"));
    mainMenu->addMenu(tr("main.toolbar.audio"));
    mainMenu->addMenu(tr("main.toolbar.input"));
    mainMenu->addMenu(tr("main.toolbar.dialog"));
    mainMenu->addMenu(tr("main.toolbar.rules"));
    mainMenu->addMenu(tr("main.toolbar.style"));
    mainMenu->addMenu(tr("main.toolbar.vfx"));
}

void MainWindow::_SetupToolBar()
{
	QToolBar* toolBar = addToolBar("Main Toolbar");
	toolBar->setMovable(false);
	toolBar->setIconSize(QSize(20, 20));

	toolBar->addAction(style()->standardIcon(QStyle::SP_FileIcon), "Tool 1");
	toolBar->addSeparator();
	toolBar->addAction(style()->standardIcon(QStyle::SP_DialogSaveButton), "Tool 2");
}

void MainWindow::_SetupCentralWidget()
{
	// RHI for main scene window
}