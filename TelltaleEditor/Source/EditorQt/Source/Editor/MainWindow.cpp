#include <Editor/MainWindow.hpp>
#include <Editor/Dialogs/SelectGameDialog.hpp>

#include <QApplication>
#include <QFont>
#include <QStyle>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QKeySequence>
#include <QMessageBox>

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
{
	setWindowTitle("Telltale Editor v0.0.1a (English, PC)");
	resize(1280, 720);
	setWindowIcon(QIcon(":/icons/LogoSquare.png"));   // icon

	// Setup the menus
	setupMenus();

	// Setup the toolbar (icons below the menu)
	setupToolBar();

	// Setup RHI (maybe here, maybe not)
	setupCentralWidget();
}

MainWindow::~MainWindow() {}

void MainWindow::setupMenus()
{
	QMenuBar* mainMenu = this->menuBar();

	auto* gameBar = mainMenu->addMenu("Game");

	auto* select = gameBar->addAction("&Select Game...");
	select->setShortcut(QKeySequence("Ctrl+Shift+G")); // Example Shortcut

	QObject::connect(select, &QAction::triggered, this, [this]() {
		SelectGameDialog dlg(this);
		if (dlg.exec() != QDialog::Accepted)
			return;
		});

	mainMenu->addMenu("File");
	mainMenu->addMenu("Editor");
	mainMenu->addMenu("Window");
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


	/*GameSnapshot snapshot{};
	CreateEditorContext(snapshot);
	Ptr<ResourceRegistry> _Registry = _Context->CreateResourceRegistry(true);
	DataStreamRef ds = _Context->LoadLibraryResource("Resources/Textures/Chore.png");
	U8* temp = TTE_ALLOC(ds->GetSize(), MEMORY_TAG_TEMPORARY);
	ds->Read(temp, 100);*/

	// Meta::GetInternalState,

	TTArchive arc{ Meta::GetInternalState().GetActiveGame().MasterArchiveVersion };
	

}

void MainWindow::setupToolBar()
{
	QToolBar* toolBar = addToolBar("Main Toolbar");
	toolBar->setMovable(true); // Looks moveable
	toolBar->setIconSize(QSize(24, 24));

	toolBar->addAction(style()->standardIcon(QStyle::SP_FileIcon), "Tool 1");

	// This is a seperator, but I don't think Telltale uses one
	toolBar->addSeparator();

	toolBar->addAction(style()->standardIcon(QStyle::SP_DialogSaveButton), "Tool 2");
}

void MainWindow::setupCentralWidget()
{
	// RHI
}