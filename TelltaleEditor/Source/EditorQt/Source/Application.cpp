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
#include <QTranslator>
#include <QLocale>
#include <QLibraryInfo>

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

	// 1. Load Qt's own translations (for standard dialogs, etc.)
	QTranslator qtTranslator;
	if (qtTranslator.load(QLocale(), "qtbase", "_",
		QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
		qtApp.installTranslator(&qtTranslator);
	}

	// 2. Load YOUR application's translations
	QTranslator appTranslator;
	if (appTranslator.load(QLocale(), "TelltaleEditor", "_", ":/i18n")) {
		qtApp.installTranslator(&appTranslator);
	}

	MainWindow window;
	window.show();

	return qtApp.exec();
}
