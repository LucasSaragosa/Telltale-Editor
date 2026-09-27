#include <Application.hpp>
#include <Editor/MainWindow.hpp>
#include <Scripting/ScriptManager.hpp>
#include <TelltaleEditor.hpp>

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
#include <QLocale>
#include <QLibraryInfo>

/*static*/ I32 Application::RunApplication(const std::vector<CommandLine::TaskArgument>& args)
{
	// Run TTE
	Application app{};
	return app._Run(args);
}

Application::Application() : QTranslator()
{

}

QString Application::translate(const char* context, const char* sourceText, const char* disambiguation, int n) const
{
    auto it = _LanguageMap.find(Symbol(sourceText));
    if (it != _LanguageMap.end())
    {
        return QString::fromUtf8(it->second);
    }

    return QString(); // return the ID string (not found)
}

void Application::SetLanguage(const String& language)
{
    if (_CurrentLanguage != language)
    {
        String langData = _Context->LoadLibraryStringResource("Resources/Language/" + language + ".txt");
        if (langData.empty())
        {
            if (CompareCaseInsensitive(language, "English"))
            {
                TTE_LOG("ERROR: Could not load language %s! Language will be using IDs instead of text!", language.c_str());
                _CurrentLanguage = "";
            }
            else
            {
                TTE_LOG("ERROR: Could not load language %s! Defaulting to english.", language.c_str());
                SetLanguage("English");
            }
            return;
        }
        _UnknownLanguageIDs.clear();
        _LanguageMap.clear();
        String line;
        std::istringstream in{ langData };
        I32 lineN = 0;
        while (std::getline(in, line))
        {
            ++lineN;
            line = StringTrim(line);
            if (StringStartsWith(line, "#") || line.empty())
                continue;
            I32 colon = (I32)line.find('=');
            if (colon == String::npos)
            {
                TTE_LOG("WARNING: At language file %s.txt:%d: invalid syntax", language.c_str(), lineN);
            }
            else
            {
                String id = StringTrim(line.substr(0, colon));
                String v = StringTrim(line.substr(colon + 1));
                while (StringStartsWith(v, "\""))
                    v = v.substr(1);
                while (StringEndsWith(v, "\""))
                    v = v.substr(0, v.length() - 1);
                _LanguageMap[Symbol(id)] = v;
            }
        }
        TTE_LOG("Loaded language %s", language.c_str());
        _CurrentLanguage = language;
    }
}

const std::set<String>& Application::GetAvailableLanguages() const
{
    return _AvailLanguages;
}

Bool Application::HasLanguageText(CString id)
{
    auto it = _LanguageMap.find(Symbol(id));
    return it != _LanguageMap.end();
}

CString Application::GetLanguageText(CString id)
{
    auto it = _LanguageMap.find(Symbol(id));
    if (it == _LanguageMap.end())
    {
        if (_UnknownLanguageIDs.count(id) == 0)
        {
            _UnknownLanguageIDs.insert(id);
            TTE_LOG("WARNING: Language ID %s has no mapping in current language %s!", id, _CurrentLanguage.c_str());
        }
        return "";
    }
    return it->second.c_str();
}

void Application::Switch(const GameSnapshot& snapshot)
{
    _Registry = nullptr;
    TelltaleEditor::Get()->Switch(snapshot);
    _Registry = TelltaleEditor::Get()->CreateResourceRegistry(true);
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

	QTranslator qtTranslator;
	if (qtTranslator.load(QLocale(), "qtbase", "_",QLibraryInfo::path(QLibraryInfo::TranslationsPath))) 
    {
		qtApp.installTranslator(&qtTranslator);
	}
    qtApp.installTranslator(this);

	_Context = CreateEditorContext({}, {}, {});

    // SETUP LANGUAGES
    String languages = _Context->LoadLibraryStringResource("Resources/Language/Languages.txt");
    String line;
    std::istringstream langstream{ languages };
    while (std::getline(langstream, line))
    {
        if (StringEndsWith(line, ".txt"))
            line = line.substr(0, line.length() - 4);
        _AvailLanguages.insert(line);
    }
    SetLanguage("English"); // or load settings etc

	// Run App
	MainWindow window{ *this };
	window.show();
	I32 code = qtApp.exec();

	FreeEditorContext();

#ifdef DEBUG
	Memory::DumpTrackedMemory(); // any memory leaks
#endif

	return code;
}
