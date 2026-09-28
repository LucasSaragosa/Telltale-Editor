#pragma once

#include <QMainWindow>
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QTreeWidget>
#include <QSplitter>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFutureWatcher>

#include <Application.hpp>
#include <Resource/TTArchive.hpp>
#include <Resource/TTArchive2.hpp> // if you have one

#include <vector>
#include <memory>

// One row in the main tree.
struct ArchiveEntryRow
{
    QString Name;
    QString Type;               // extension without dot
    qint64  CompressedSize = -1;
    qint64  UncompressedSize = 0;
    quint64 Offset = 0;
    QString BlowfishKeyLabel;   // which game/key name worked
};

// One row in the summary table.
struct ArchiveSummaryRow
{
    QString Type;
    int     Count = 0;
    qint64  TotalCompressedSize = 0;
    qint64  TotalUncompressedSize = 0;
};

class ArchiveBrowserWindow : public QMainWindow
{
    Q_OBJECT

public:

    explicit ArchiveBrowserWindow(Application& app, QWidget* parent = nullptr);
    ~ArchiveBrowserWindow() override;

private slots:

    void OnOpenArchive();

private:

    // Tries every known Blowfish key (master + per-snapshot) across every
    // registered game until one loads. Returns the loaded archive on success,
    // sets outKeyLabel to a human-readable identifier of the key that worked.
    std::unique_ptr<TTArchive> TryLoadArchive(const QString& filePath,
        QString& outKeyLabel,
        QString& outError);

    // Populates the tree + summary from a successfully loaded archive.
    void BuildEntries(TTArchive& archive);
    void UpdateSummary();
    void UpdateInfoPanel(TTArchive& archive);

    // Filters the visible tree rows by substring.
    void FilterEntries(const QString& filter);

    // Rough per-file compressed size estimator using chunk sizes (mirrors the
    // C# EstimateCompressedSize).
    quint64 EstimateCompressedSize(quint64 offset, quint64 size,
        const std::vector<U64>& chunkBlockSizes,
        quint64 windowSize) const;

    Application& _App;

    // --- UI ---
    QLabel* _NameValue = nullptr;
    QLabel* _UncompValue = nullptr;
    QLabel* _CompValue = nullptr;
    QLabel* _CompressionVal = nullptr;
    QLabel* _VersionVal = nullptr;
    QLabel* _KeyVal = nullptr;
    QLabel* _EncryptionVal = nullptr;
    QLineEdit* _SearchBox = nullptr;
    QTreeWidget* _MainTree = nullptr;
    QTreeWidget* _SummaryTree = nullptr;

    // --- Data ---
    std::vector<ArchiveEntryRow>   _AllEntries;     // unfiltered
    std::vector<ArchiveEntryRow>   _Entries;        // currently displayed
    std::vector<ArchiveSummaryRow> _Summary;

    std::unique_ptr<TTArchive> _LoadedArchive;
    QString _LoadedFilePath;
    QString _UsedBlowfishKeyLabel;
};