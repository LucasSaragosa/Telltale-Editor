#include <Application.hpp>
#include <Editor/MainWindow.hpp>

#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSizePolicy>
#include <QFont>

/*static*/ I32 Application::RunApplication(const std::vector<CommandLine::TaskArgument>& args)
{
	// Run TTE
	Application app{};
	return app._Run(args);
}

I32 Application::_Run(const std::vector<CommandLine::TaskArgument>& args)
{
	int argc = 0;
	char** argv = nullptr;

	QApplication qtApp(argc, argv);

	MainWindow window;
	window.show();

	return qtApp.exec();

	// ------------------------------------------------------------
	// Project Selection
	// ------------------------------------------------------------

	QMainWindow projectSelectionWindow;
	projectSelectionWindow.setWindowTitle("Telltale Editor v0.0.1a");
	projectSelectionWindow.resize(1200, 700);

	// Main content
	auto* centralWidget = new QWidget(&projectSelectionWindow);
	auto* mainLayout = new QHBoxLayout(centralWidget);

	// ============================================================
	// Left: New Project
	// ============================================================

	auto* newProjectPanel = new QWidget(centralWidget);
	auto* newProjectLayout = new QVBoxLayout(newProjectPanel);

	auto* newProjectButton = new QPushButton("+\nNew Project", newProjectPanel);

	newProjectButton->setMinimumSize(400, 400);
	newProjectButton->setSizePolicy(
		QSizePolicy::Expanding,
		QSizePolicy::Expanding);

	QFont newProjectFont = newProjectButton->font();
	newProjectFont.setPointSize(20);
	newProjectButton->setFont(newProjectFont);

	newProjectLayout->addWidget(
		newProjectButton,
		1,
		Qt::AlignCenter);

	auto* recentPanel = new QWidget(centralWidget);
	auto* recentLayout = new QVBoxLayout(recentPanel);

	auto* recentTitle = new QLabel("Recent Projects", recentPanel);

	QFont titleFont = recentTitle->font();
	titleFont.setPointSize(18);
	titleFont.setBold(true);
	recentTitle->setFont(titleFont);

	recentLayout->addWidget(recentTitle);

	auto* recentProjects = new QListWidget(recentPanel);
	recentProjects->setSizePolicy(
		QSizePolicy::Expanding,
		QSizePolicy::Expanding);

	// Temporary data.
	// These will eventually come from your project manager/config.
	recentProjects->addItem("Project Alpha");
	recentProjects->addItem("Project Beta");
	recentProjects->addItem("Project Gamma");
	recentProjects->addItem("Project Delta");

	recentLayout->addWidget(recentProjects, 1);

	// ============================================================
	// 75 / 25 split
	// ============================================================

	mainLayout->addWidget(newProjectPanel, 3);
	mainLayout->addWidget(recentPanel, 1);

	projectSelectionWindow.setCentralWidget(centralWidget);

	QObject::connect(newProjectButton, &QPushButton::clicked, [&]()
		{
			// TODO:
			// Open your New Project window/dialog.
		});

	QObject::connect(recentProjects, &QListWidget::itemDoubleClicked, [&](QListWidgetItem* item)
		{
			if (!item)
				return;

			// TODO:
			// Open the selected project.
		});

	projectSelectionWindow.show();

	return qtApp.exec();
}
