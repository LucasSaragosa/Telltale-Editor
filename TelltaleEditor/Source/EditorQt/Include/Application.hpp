#pragma once

#include <TelltaleEditor.hpp>

#include <set>
#include <map>

#include <QTranslator>
#include <QString>

class Application : public QTranslator
{
public:

    virtual QString translate(const char* context, const char* sourceText, const char* disambiguation = nullptr, int n = -1) const override;

    Application();

	static I32 RunApplication(const std::vector<CommandLine::TaskArgument>& args); // runs the application (from command line)

    inline Ptr<ResourceRegistry> GetResourceRegistry()
    {
        return _Registry;
    }

    // LANGUAGE
    void SetLanguage(const String& lang);
    const std::set<String>& GetAvailableLanguages() const;
    Bool HasLanguageText(CString id);
    CString GetLanguageText(CString id);

    void Switch(const GameSnapshot& snapshot);

    inline GameSnapshot GetSnapshot()
    {
        return _Snapshot;
    }

    inline Bool GameInitialised()
    {
        return _GameInit;
    }

protected:

	I32 _Run(const std::vector<CommandLine::TaskArgument>& args);

private:

    TelltaleEditor* _Context = nullptr;
    Ptr<ResourceRegistry> _Registry;
    GameSnapshot _Snapshot;
    Bool _GameInit = false;

    // LANGUAGE
    String _CurrentLanguage;
    std::set<String> _AvailLanguages;
    std::map<U64, String> _LanguageMap;
    std::set<String> _UnknownLanguageIDs;

};