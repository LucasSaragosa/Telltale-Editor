#include <Editor/Window/ArchiveBrowser.hpp>

#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QLineEdit>

#include <Meta/Meta.hpp>
#include <Resource/DataStream.hpp>

// ===================================================================
// Helpers
// ===================================================================

static QString FormatSize(qint64 bytes)
{
    if (bytes < 0)
        return QStringLiteral("N/A");

    static const char* suf[] = { "B", "KB", "MB", "GB", "TB" };
    int place = 0;
    double size = double(bytes);
    while (size >= 1024.0 && place < 4)
    {
        size /= 1024.0;
        place++;
    }
    return QString::number(size, 'f', 2) + ' ' + suf[place];
}

static QString ExtensionOf(const QString& name)
{
    const int dot = name.lastIndexOf('.');
    return dot < 0 ? QString() : name.mid(dot + 1).toLower();
}

// TTArchive via SetBlowfishKey and see if it parses.
static Bool TryLoadWithKey(TTArchive& archive, DataStreamRef& in, const Meta::BlowfishKey& key)
{
    archive.Reset();
    archive.SetBlowfishKey(key.BfKey, key.BfKeyLength);
    in->SetPosition(0);
    return archive.SerialiseIn(in);
}

static QString MakeKeyLabel(const Meta::RegGame& game, const String& snapName, const Meta::BlowfishKey& key)
{
    if (key.BfKeyLength == 0)
        return QString::fromUtf8(game.Name) + " (no key)";
    if (snapName.empty())
        return  QString::fromUtf8(game.Name) + " (master)";
    return  QString::fromUtf8(game.Name) + " / " + QString::fromUtf8(snapName);
}

// ===================================================================
// Window
// ===================================================================

ArchiveBrowserWindow::ArchiveBrowserWindow(Application& app, QWidget* parent)
    : QMainWindow(parent), _App(app)
{
    setWindowTitle(tr("Archive Browser"));
    resize(1200, 700);

    // ---- Menu bar ----
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
    QAction* openAct = fileMenu->addAction(tr("&Open Archive..."));
    connect(openAct, &QAction::triggered, this, &ArchiveBrowserWindow::OnOpenArchive);
    fileMenu->addSeparator();
    QAction* quitAct = fileMenu->addAction(tr("&Quit"));
    connect(quitAct, &QAction::triggered, this, &QWidget::close);

    // ---- Central layout ----
    auto* central = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(central);

    auto* splitter = new QSplitter(Qt::Horizontal, central);
    rootLayout->addWidget(splitter, 1);

    // ---- Left column: info panel + summary ----
    auto* leftCol = new QWidget(splitter);
    auto* leftLayout = new QVBoxLayout(leftCol);

    auto* infoBox = new QGroupBox(tr("Archive Info"), leftCol);
    auto* infoGrid = new QGridLayout(infoBox);

    auto addInfoRow = [&](int row, const QString& label, QLabel*& outValue)
        {
            infoGrid->addWidget(new QLabel(label, infoBox), row, 0);
            outValue = new QLabel(QString(), infoBox);
            outValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            infoGrid->addWidget(outValue, row, 1);
        };

    addInfoRow(0, tr("Name:"), _NameValue);
    addInfoRow(1, tr("Uncompressed Size:"), _UncompValue);
    addInfoRow(2, tr("Compressed Size:"), _CompValue);
    addInfoRow(3, tr("Compression:"), _CompressionVal);
    addInfoRow(4, tr("Version:"), _VersionVal);
    addInfoRow(5, tr("Blowfish Key:"), _KeyVal);
    addInfoRow(6, tr("Encryption:"), _EncryptionVal);

    leftLayout->addWidget(infoBox);

    _SummaryTree = new QTreeWidget(leftCol);
    _SummaryTree->setColumnCount(5);
    _SummaryTree->setHeaderLabels({ tr("Type"), tr("Count"),
                                    tr("Compressed"), tr("Uncompressed"), tr("Compressed %") });
    _SummaryTree->setRootIsDecorated(false);
    _SummaryTree->setAlternatingRowColors(true);
    _SummaryTree->setSortingEnabled(true);
    leftLayout->addWidget(_SummaryTree, 1);

    // ---- Right column: search + entries ----
    auto* rightCol = new QWidget(splitter);
    auto* rightLayout = new QVBoxLayout(rightCol);

    _SearchBox = new QLineEdit(rightCol);
    _SearchBox->setPlaceholderText(tr("Filter by name..."));
    connect(_SearchBox, &QLineEdit::textChanged,
        this, &ArchiveBrowserWindow::FilterEntries);
    rightLayout->addWidget(_SearchBox);

    _MainTree = new QTreeWidget(rightCol);
    _MainTree->setColumnCount(5);
    _MainTree->setHeaderLabels({ tr("Name"), tr("Type"),
                                 tr("Compressed"), tr("Uncompressed"), tr("Position") });
    _MainTree->setRootIsDecorated(false);
    _MainTree->setAlternatingRowColors(true);
    _MainTree->setSortingEnabled(true);
    rightLayout->addWidget(_MainTree, 1);

    // Double-click to open a resource (placeholder).
    connect(_MainTree, &QTreeWidget::itemDoubleClicked,
        this, [this](QTreeWidgetItem* item, int)
        {
            if (!item) return;
            const int idx = item->data(0, Qt::UserRole).toInt();
            if (idx < 0 || idx >= int(_Entries.size())) return;
            const auto& entry = _Entries[idx];

            if (!_LoadedArchive) return;
            DataStreamRef stream = _LoadedArchive->Find(Symbol(entry.Name.toStdString()), nullptr);

        });

    splitter->addWidget(leftCol);
    splitter->addWidget(rightCol);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    setCentralWidget(central);
}

ArchiveBrowserWindow::~ArchiveBrowserWindow() = default;

// ===================================================================
// Open archive
// ===================================================================

void ArchiveBrowserWindow::OnOpenArchive()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Select Telltale Archive"), QString(),
        tr("Telltale Archives (*.ttarch *.ttarch2);;All Files (*)"));

    if (path.isEmpty())
        return;

    QString keyLabel;
    QString error;
    std::unique_ptr<TTArchive> archive = TryLoadArchive(path, keyLabel, error);

    if (!archive)
    {
        QMessageBox::warning(this, tr("Load Failed"),
            tr("Could not load archive.\nLast error: %1").arg(error));
        return;
    }

    _LoadedArchive = std::move(archive);
    _LoadedFilePath = path;
    _UsedBlowfishKeyLabel = keyLabel;

    setWindowTitle(tr("Archive Browser - %1").arg(QFileInfo(path).fileName()));

    BuildEntries(*_LoadedArchive);
    UpdateSummary();
    UpdateInfoPanel(*_LoadedArchive);
}

// ===================================================================
// Key discovery
// ===================================================================

std::unique_ptr<TTArchive> ArchiveBrowserWindow::TryLoadArchive(const QString& filePath,
    QString& outKeyLabel,
    QString& outError)
{
    // Open the file once and wrap it in a DataStreamRef.
    DataStreamRef in = DataStreamManager::GetInstance()->CreateFileStream(
        ResourceURL(ResourceScheme::FILE, filePath.toStdString()));

    if (!in)
    {
        outError = tr("Could not open file.");
        return nullptr;
    }

    const Meta::InternalState& state = Meta::GetInternalState();

    // Collect every unique key we know about, with a label. Deduplicate so
    // the same master key shared across many games is only tried once.
    struct KeyCandidate
    {
        Meta::BlowfishKey Key;
        QString Label;
    };
    std::vector<KeyCandidate> candidates;

    for (const Meta::RegGame& game : state.Games)
    {
        // Master key.
        if (game.MasterKey.BfKeyLength > 0)
        {
            bool dup = false;
            for (const auto& c : candidates)
            {
                if (c.Key.BfKeyLength == game.MasterKey.BfKeyLength &&
                    memcmp(c.Key.BfKey, game.MasterKey.BfKey, game.MasterKey.BfKeyLength) == 0)
                {
                    dup = true; break;
                }
            }
            if (!dup)
                candidates.push_back({ game.MasterKey, MakeKeyLabel(game, String{}, game.MasterKey) });
        }

        // Per-snapshot keys.
        for (const auto& kv : game.SnapToEncryptionKey)
        {
            const Meta::BlowfishKey& k = kv.second;
            if (k.BfKeyLength == 0)
                continue;

            bool dup = false;
            for (const auto& c : candidates)
            {
                if (c.Key.BfKeyLength == k.BfKeyLength &&
                    memcmp(c.Key.BfKey, k.BfKey, k.BfKeyLength) == 0)
                {
                    dup = true; break;
                }
            }
            if (!dup)
                candidates.push_back({ k, MakeKeyLabel(game, kv.first, k) });
        }
    }

    // Always include the empty key first (unencrypted archives).
    candidates.insert(candidates.begin(), { Meta::BlowfishKey{}, tr("None (unencrypted)") });

    // Now try each one. Each attempt re-uses the same TTArchive instance
    // (Reset() between tries) so we don't allocate a new parser for every key.
    std::unique_ptr<TTArchive> archive = std::make_unique<TTArchive>(0);
    QString lastErr;

    for (size_t i = 0; i < candidates.size(); i++)
    {
        if (TryLoadWithKey(*archive, in, candidates[i].Key))
        {
            outKeyLabel = candidates[i].Label;
            return archive;
        }

        lastErr = tr("Key '%1' failed.").arg(candidates[i].Label);
    }

    outError = lastErr;
    return nullptr;
}

// ===================================================================
// Entries
// ===================================================================

void ArchiveBrowserWindow::BuildEntries(TTArchive& archive)
{
    _AllEntries.clear();

    std::set<String> names;
    archive.GetFiles(names);

    // Grab chunk info for the compressed-size estimate. TTArchive exposes
    // _Files privately; you may want to add a public accessor. Until then,
    // we just use uncompressed size as the compressed size (best effort).
    //
    // If you expose: archive.GetChunkBlockSizes() and archive.GetChunkSize(),
    // pass them to EstimateCompressedSize below.
    const std::vector<U64> chunkSizes;    // empty for now
    const U64 windowSize = 0x10000;

    for (const String& name : names)
    {
        DataStreamRef stream = archive.Find(Symbol(name), nullptr);
        const quint64 size = stream ? stream->GetSize() : 0;

        ArchiveEntryRow row;
        row.Name = QString::fromStdString(name);
        row.Type = ExtensionOf(row.Name);
        row.UncompressedSize = qint64(size);
        row.CompressedSize = qint64(EstimateCompressedSize(0, size, chunkSizes, windowSize));
        row.Offset = 0; // available if you expose per-entry offsets
        row.BlowfishKeyLabel = _UsedBlowfishKeyLabel;
        _AllEntries.push_back(std::move(row));
    }

    std::sort(_AllEntries.begin(), _AllEntries.end(),
        [](const ArchiveEntryRow& a, const ArchiveEntryRow& b)
        {
            return a.Name.compare(b.Name, Qt::CaseInsensitive) < 0;
        });

    FilterEntries(QString());
    _SearchBox->clear();
}

void ArchiveBrowserWindow::FilterEntries(const QString& filter)
{
    _Entries.clear();
    if (filter.isEmpty())
    {
        _Entries = _AllEntries;
    }
    else
    {
        for (const auto& e : _AllEntries)
        {
            if (e.Name.contains(filter, Qt::CaseInsensitive))
                _Entries.push_back(e);
        }
    }

    _MainTree->clear();
    for (int i = 0; i < int(_Entries.size()); i++)
    {
        const auto& e = _Entries[i];
        auto* item = new QTreeWidgetItem(_MainTree);
        item->setText(0, e.Name);
        item->setText(1, e.Type);
        item->setText(2, FormatSize(e.CompressedSize));
        item->setText(3, FormatSize(e.UncompressedSize));
        item->setText(4, QString::number(e.Offset));
        item->setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);
        item->setTextAlignment(3, Qt::AlignRight | Qt::AlignVCenter);
        item->setTextAlignment(4, Qt::AlignRight | Qt::AlignVCenter);
        item->setData(0, Qt::UserRole, i);
    }

    for (int c = 0; c < _MainTree->columnCount(); c++)
        _MainTree->resizeColumnToContents(c);

    UpdateSummary();
}

// ===================================================================
// Summary
// ===================================================================

void ArchiveBrowserWindow::UpdateSummary()
{
    std::map<QString, ArchiveSummaryRow> grouped;
    for (const auto& e : _Entries)
    {
        const QString key = e.Type.isEmpty() ? tr("(no extension)") : e.Type;
        ArchiveSummaryRow& row = grouped[key];
        row.Type = key;
        row.Count++;
        row.TotalCompressedSize += e.CompressedSize;
        row.TotalUncompressedSize += e.UncompressedSize;
    }

    _Summary.clear();
    for (auto& kv : grouped)
        _Summary.push_back(std::move(kv.second));

    std::sort(_Summary.begin(), _Summary.end(),
        [](const ArchiveSummaryRow& a, const ArchiveSummaryRow& b)
        {
            return a.Type.compare(b.Type, Qt::CaseInsensitive) < 0;
        });

    // Total compressed across all displayed, for the percent bar.
    qint64 totalComp = 0;
    for (const auto& r : _Summary)
        totalComp += r.TotalCompressedSize;

    _SummaryTree->clear();
    for (const auto& r : _Summary)
    {
        auto* item = new QTreeWidgetItem(_SummaryTree);
        item->setText(0, r.Type);
        item->setText(1, QString::number(r.Count));
        item->setText(2, FormatSize(r.TotalCompressedSize));
        item->setText(3, FormatSize(r.TotalUncompressedSize));

        double percent = totalComp > 0
            ? double(r.TotalCompressedSize) / double(totalComp) * 100.0
            : 0.0;
        item->setText(4, QString::number(percent, 'f', 3) + '%');

        item->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
        item->setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);
        item->setTextAlignment(3, Qt::AlignRight | Qt::AlignVCenter);
        item->setTextAlignment(4, Qt::AlignRight | Qt::AlignVCenter);
    }

    for (int c = 0; c < _SummaryTree->columnCount(); c++)
        _SummaryTree->resizeColumnToContents(c);
}

// ===================================================================
// Info panel
// ===================================================================

void ArchiveBrowserWindow::UpdateInfoPanel(TTArchive& archive)
{
    qint64 totalComp = 0, totalUncomp = 0;
    for (const auto& e : _Entries)
    {
        totalComp += e.CompressedSize;
        totalUncomp += e.UncompressedSize;
    }

    _NameValue->setText(QFileInfo(_LoadedFilePath).fileName());
    _UncompValue->setText(FormatSize(totalUncomp));
    _CompValue->setText(FormatSize(totalComp));
    _CompressionVal->setText(tr("Unknown")); // expose compression from TTArchive if you track it
    // _VersionVal->setText(QString::number(archive.GetVersion()));
    _KeyVal->setText(_UsedBlowfishKeyLabel);
    _EncryptionVal->setText(_UsedBlowfishKeyLabel == tr("None (unencrypted)")
        ? tr("No") : tr("Yes"));
}

// ===================================================================
// Compressed size estimate
// ===================================================================

quint64 ArchiveBrowserWindow::EstimateCompressedSize(quint64 offset, quint64 size,
    const std::vector<U64>& chunkBlockSizes,
    quint64 windowSize) const
{
    if (chunkBlockSizes.empty())
        return size; // no compression

    if (size == 0)
        return 0;

    const quint64 fileStart = offset;         // in uncompressed payload space
    const quint64 fileEnd = offset + size;  // exclusive

    const int startChunk = int(fileStart / windowSize);
    const int endChunk = int((fileEnd - 1) / windowSize);

    if (endChunk >= int(chunkBlockSizes.size()))
        return 0;

    double total = 0.0;
    for (int i = startChunk; i <= endChunk; i++)
    {
        const quint64 chunkStart = quint64(i) * windowSize;
        const quint64 chunkEnd = quint64(i + 1) * windowSize;

        const quint64 overlapStart = std::max(fileStart, chunkStart);
        const quint64 overlapEnd = std::min(fileEnd, chunkEnd);
        const quint64 overlapSize = overlapEnd - overlapStart;

        const double ratio = double(overlapSize) / double(windowSize);
        total += ratio * double(chunkBlockSizes[i]);
    }

    return quint64(std::ceil(total));
}