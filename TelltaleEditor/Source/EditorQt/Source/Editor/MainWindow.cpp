#include <Editor/MainWindow.hpp>
#include <Editor/Dialogs/SelectGameDialog.hpp>
#include <Editor/Window/ArchiveBrowser.hpp>
#include <Editor/Window/ResourceLocations.hpp>

#include <QApplication>
#include <QFont>
#include <QStyle>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QKeySequence>
#include <QMessageBox>
#include <QWindow>

static MainWindow* _Mw = nullptr;
Ptr<ResourceRegistry> GetEditorResourceRegistry()
{
	return _Mw ? _Mw->GetResourceRegistry() : nullptr;
}

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
{
	setWindowTitle("Telltale Editor " TTE_VERSION " (No Active Game)");
	resize(1280, 720);
	setWindowIcon(QIcon(":/icons/LogoSquare.png"));

	// Setup the menus
	_SetupMenus();

	// Setup the toolbar (icons below the menu)
	_SetupToolBar();

	// Setup RHI (maybe here, maybe not)
	_SetupCentralWidget();

	_Mw = this;
}

MainWindow::~MainWindow() {}

void MainWindow::_SetupMenus()
{
	QMenuBar* mainMenu = this->menuBar();

	auto* gameBar = mainMenu->addMenu("Game");

	auto* select = gameBar->addAction("&Select Game...");

    QObject::connect(select, &QAction::triggered, this, [this]() 
	{
        SelectGameDialog dlg(this);
        if (dlg.exec() != QDialog::Accepted)
            return;

        const GameSnapshot& snapshot = dlg.GetSnapshot();

		_Registry = nullptr;
		TelltaleEditor::Get()->Switch(snapshot);
		_Registry = TelltaleEditor::Get()->CreateResourceRegistry(true);

        setWindowTitle(QString("Telltale Editor " TTE_VERSION " %1 (%2)")
            .arg(QString::fromStdString(snapshot.ID),
            QString::fromStdString(snapshot.Platform)));
    });

	mainMenu->addMenu("File");
	mainMenu->addMenu("Editor");

	// ======================== WINDOW SUBMENU =======================

	auto *windowMenu = mainMenu->addMenu("Window");

	// ARCHIVE BROWSER
	auto* arcBrowser = windowMenu->addAction("&Archive Browser");
	QObject::connect(arcBrowser, &QAction::triggered, this, [this]()
    {
        ArchiveBrowserWindow w(this);
		w.exec();
    });

    // RESOURCE LOCATIONS
    auto* resourceLocs = windowMenu->addAction("&Resource Locations");
    QObject::connect(resourceLocs, &QAction::triggered, this, [this]()
    {
        ResourceLocationsWindow w(this);
		w.exec();
    });

	// ===============================================================

	mainMenu->addMenu("Scripts");
	mainMenu->addMenu("Scene");
	mainMenu->addMenu("Properties");
	mainMenu->addMenu("Choreography");
	mainMenu->addMenu("Audio");
	mainMenu->addMenu("Input");
	mainMenu->addMenu("Dialog");
	mainMenu->addMenu("Rules");
	mainMenu->addMenu("Style");
	mainMenu->addMenu("Vfx");
}

void MainWindow::_SetupToolBar()
{
	QToolBar* toolBar = addToolBar("Main Toolbar");
	toolBar->setMovable(true); // Looks moveable
	toolBar->setIconSize(QSize(20, 20));

	toolBar->addAction(style()->standardIcon(QStyle::SP_FileIcon), "Tool 1");

	// This is a seperator, but I don't think Telltale uses one
	toolBar->addSeparator();

	toolBar->addAction(style()->standardIcon(QStyle::SP_DialogSaveButton), "Tool 2");
}

void MainWindow::_SetupCentralWidget()
{
	// RHI
}