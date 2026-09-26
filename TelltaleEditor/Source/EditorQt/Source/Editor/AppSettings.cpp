#include <Editor/AppSettings.hpp>

#include <QSettings>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>

// ---------- Keys (centralized) ----------
namespace Keys 
{
    static constexpr auto GameFolder = "game/folder";
    static constexpr auto ExecutablePath = "game/executable";
    static constexpr auto GameId = "game/id";
    static constexpr auto Platform = "game/platform";
    static constexpr auto Vendor = "game/vendor";

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
        : settings(QSettings::IniFormat, QSettings::UserScope, "TelltaleEditor", "TelltaleEditor")
    {
        settings.setFallbacksEnabled(false);
    }
};

AppSettings& AppSettings::Get()
{
    static AppSettings instance;
    return instance;
}

AppSettings::AppSettings()
    : _MyImpl(new Impl())
{
}

QString AppSettings::GetGameFolder() const
{
    return _MyImpl->settings.value(Keys::GameFolder).toString();
}

void AppSettings::SetGameFolder(const QString& path)
{
    _MyImpl->settings.setValue(Keys::GameFolder, path);
}

QString AppSettings::GetExecutablePath() const
{
    return _MyImpl->settings.value(Keys::ExecutablePath).toString();
}

void AppSettings::SetExecutablePath(const QString& path)
{
    _MyImpl->settings.setValue(Keys::ExecutablePath, path);
}

QString AppSettings::GetSelectedGameId() const
{
    return _MyImpl->settings.value(Keys::GameId).toString();
}

void AppSettings::SetSelectedGameId(const QString& id)
{
    _MyImpl->settings.setValue(Keys::GameId, id);
}

QString AppSettings::GetSelectedPlatform() const
{
    return _MyImpl->settings.value(Keys::Platform).toString();
}

void AppSettings::SetSelectedPlatform(const QString& p)
{
    _MyImpl->settings.setValue(Keys::Platform, p);
}

QString AppSettings::GetSelectedVendor() const
{
    return _MyImpl->settings.value(Keys::Vendor).toString();
}

void AppSettings::SetSelectedVendor(const QString& v)
{
    _MyImpl->settings.setValue(Keys::Vendor, v);
}

QStringList AppSettings::GetRecentProjects() const
{
    return _MyImpl->settings.value(Keys::RecentProjects).toStringList();
}

void AppSettings::AddRecentProject(const QString& path)
{
    auto list = GetRecentProjects();
    list.removeAll(path);
    list.prepend(path);
    while (list.size() > 10) 
        list.removeLast();     // keep it tidy
    _MyImpl->settings.setValue(Keys::RecentProjects, list);
}

void AppSettings::ClearRecentProjects()
{
    _MyImpl->settings.remove(Keys::RecentProjects);
}

QByteArray AppSettings::GetMainWindowGeometry() const
{
    return _MyImpl->settings.value(Keys::MainGeometry).toByteArray();
}

void AppSettings::SetMainWindowGeometry(const QByteArray& geo)
{
    _MyImpl->settings.setValue(Keys::MainGeometry, geo);
}

QByteArray AppSettings::GetMainWindowState() const
{
    return _MyImpl->settings.value(Keys::MainState).toByteArray();
}

void AppSettings::SetMainWindowState(const QByteArray& state)
{
    _MyImpl->settings.setValue(Keys::MainState, state);
}

// ---------- Generic ----------
QVariant AppSettings::Get(const QString& key, const QVariant& def) const
{
    return _MyImpl->settings.value(key, def);
}

void AppSettings::Set(const QString& key, const QVariant& value)
{
    _MyImpl->settings.setValue(key, value);
}

void AppSettings::Sync()
{
    _MyImpl->settings.sync();
}

void AppSettings::ClearGameSelection()
{
    _MyImpl->settings.remove(Keys::GameFolder);
    _MyImpl->settings.remove(Keys::ExecutablePath);
    _MyImpl->settings.remove(Keys::GameId);
    _MyImpl->settings.remove(Keys::Platform);
    _MyImpl->settings.remove(Keys::Vendor);
}