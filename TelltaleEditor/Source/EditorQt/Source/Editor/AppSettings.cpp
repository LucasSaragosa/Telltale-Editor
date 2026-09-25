#include <Editor/AppSettings.hpp>

#include <QSettings>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>

// ---------- Keys (centralized) ----------
namespace Keys {
    static constexpr auto GameFolder = "game/folder";
    static constexpr auto ExecutablePath = "game/executable";
    static constexpr auto GameId = "game/id";
    static constexpr auto Platform = "game/platform";

    static constexpr auto RecentProjects = "projects/recent";

    static constexpr auto MainGeometry = "window/main/geometry";
    static constexpr auto MainState = "window/main/state";
}

// ---------- Impl ----------
class AppSettings::Impl
{
public:
    QSettings settings;

    Impl()
        : settings(
            // Organization + app name → drives where files/registry keys land
            QSettings::IniFormat,                       // force INI for portability
            QSettings::UserScope,
            "TelltaleEditor",                           // org
            "TelltaleEditor")                           // app
    {
        // Optional: store next to the executable instead of in AppData
        // settings.setPath(QSettings::IniFormat, QSettings::UserScope,
        //                  QCoreApplication::applicationDirPath());
        settings.setFallbacksEnabled(false);
    }
};

// ---------- Singleton ----------
AppSettings& AppSettings::Get()
{
    static AppSettings instance;
    return instance;
}

AppSettings::AppSettings()
    : m_impl(new Impl())
{
}

// ---------- Typed getters/setters ----------
QString AppSettings::GetGameFolder() const
{
    return m_impl->settings.value(Keys::GameFolder).toString();
}

void AppSettings::SetGameFolder(const QString& path)
{
    m_impl->settings.setValue(Keys::GameFolder, path);
}

QString AppSettings::GetExecutablePath() const
{
    return m_impl->settings.value(Keys::ExecutablePath).toString();
}

void AppSettings::SetExecutablePath(const QString& path)
{
    m_impl->settings.setValue(Keys::ExecutablePath, path);
}

QString AppSettings::GetSelectedGameId() const
{
    return m_impl->settings.value(Keys::GameId).toString();
}

void AppSettings::SetSelectedGameId(const QString& id)
{
    m_impl->settings.setValue(Keys::GameId, id);
}

QString AppSettings::GetSelectedPlatform() const
{
    return m_impl->settings.value(Keys::Platform).toString();
}

void AppSettings::SetSelectedPlatform(const QString& p)
{
    m_impl->settings.setValue(Keys::Platform, p);
}

QStringList AppSettings::GetRecentProjects() const
{
    return m_impl->settings.value(Keys::RecentProjects).toStringList();
}

void AppSettings::AddRecentProject(const QString& path)
{
    auto list = GetRecentProjects();
    list.removeAll(path);
    list.prepend(path);
    while (list.size() > 10) list.removeLast();     // keep it tidy
    m_impl->settings.setValue(Keys::RecentProjects, list);
}

void AppSettings::ClearRecentProjects()
{
    m_impl->settings.remove(Keys::RecentProjects);
}

QByteArray AppSettings::GetMainWindowGeometry() const
{
    return m_impl->settings.value(Keys::MainGeometry).toByteArray();
}

void AppSettings::SetMainWindowGeometry(const QByteArray& geo)
{
    m_impl->settings.setValue(Keys::MainGeometry, geo);
}

QByteArray AppSettings::GetMainWindowState() const
{
    return m_impl->settings.value(Keys::MainState).toByteArray();
}

void AppSettings::SetMainWindowState(const QByteArray& state)
{
    m_impl->settings.setValue(Keys::MainState, state);
}

// ---------- Generic ----------
QVariant AppSettings::Get(const QString& key, const QVariant& def) const
{
    return m_impl->settings.value(key, def);
}

void AppSettings::Set(const QString& key, const QVariant& value)
{
    m_impl->settings.setValue(key, value);
}

void AppSettings::Sync()
{
    m_impl->settings.sync();
}