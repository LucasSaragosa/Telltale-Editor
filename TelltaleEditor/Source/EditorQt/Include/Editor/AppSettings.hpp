#pragma once

#include <QString>
#include <QSize>
#include <QByteArray>

class AppSettings
{
public:
    // Singleton access
    static AppSettings& Get();

    // ---- Game / project ----
    QString GetGameFolder() const;
    void    SetGameFolder(const QString& path);

    QString GetExecutablePath() const;
    void    SetExecutablePath(const QString& path);

    QString GetSelectedGameId() const;
    void    SetSelectedGameId(const QString& id);

    QString GetSelectedPlatform() const;
    void    SetSelectedPlatform(const QString& p);

    // ---- Recent projects ----
    QStringList GetRecentProjects() const;
    void        AddRecentProject(const QString& path);
    void        ClearRecentProjects();

    // ---- Window state ----
    QByteArray GetMainWindowGeometry() const;
    void       SetMainWindowGeometry(const QByteArray& geo);
    QByteArray GetMainWindowState() const;
    void       SetMainWindowState(const QByteArray& state);

    // ---- Generic escape hatches (for ad-hoc values) ----
    QVariant Get(const QString& key, const QVariant& def) const;
    void     Set(const QString& key, const QVariant& value);
    void     Sync();

private:
    AppSettings();
    ~AppSettings() = default;
    AppSettings(const AppSettings&) = delete;
    AppSettings& operator=(const AppSettings&) = delete;

    class Impl;
    Impl* m_impl;   // pimpl so QSettings header doesn't leak everywhere
};