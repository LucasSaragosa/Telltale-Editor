#include <Application.hpp>
#include <Editor/MainWindow.hpp>
#include <Scripting/ScriptManager.hpp>

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
//
//void luaModuleUI(LuaFunctionCollection& Col)
//{
//	PUSH_FUNC(Col, "RegisterModuleUI", &luaRegisterModuleUI, "nil RegisterModuleUI(moduleID, moduleImage, dataTable)", "Registers data which describes how the inspector view should render this module."
//		" Data table is a table of UI element to a table of that elements info. Class, PropKey and Default are required keys. In each entry, 'UI' is a required table which describes how to render it."
//		" It should contain InputType which is a string of the valid kPropRenderXXX types. Or 'handle:xxx' where xxx is the *extension* of the file name to have a handle set as. Note in this case that"
//		" the handle property must be a string or symbol. Along with InputType, SubPath is a string. By default set it to 'this', else a chain of variables to get to the input type variable, eg this.mHandle"
//		" if the type is a handle and a string is required if mHandle is a string in the Handle class specifying the file name. There can be as many this.sub.paths.as.needed .");
//
//	const PropertyRenderInstruction* Instruction = PropertyRenderInstructions;
//	while (Instruction->Name)
//	{
//		PUSH_GLOBAL_S(Col, Instruction->ConstantName, Instruction->Name, "Module property render instructions");
//		Instruction++;
//	}
// }

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

	CreateEditorContext({}, {}, {});

	MainWindow window;
	window.show();

	return qtApp.exec();
}
