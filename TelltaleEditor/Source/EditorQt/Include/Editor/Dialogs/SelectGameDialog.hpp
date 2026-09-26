#pragma once

#include <QDialog>

struct GameSnapshot;

class QLineEdit;
class QComboBox;
class QPushButton;
class QLabel;
class QDialogButtonBox;

class SelectGameDialog : public QDialog 
{
    Q_OBJECT
public:

    explicit SelectGameDialog(QWidget* parent = nullptr);
    ~SelectGameDialog() override;

    const GameSnapshot& GetSnapshot() const { return *_ActiveSnapshot; }

private slots:

    void _BrowseForExecutable();
    void _OnChangeExecutable(const QString& path);
    void _OnGameChange(int index);
    void _OnAccept();

private:

    void _PopulateGameCombos();
    void _DetectGameFromPath(const QString& exePath);
    void _UpdateAcceptState();
    bool _ValidateInputs(QString* outError) const;
    void _RestoreFromSettings();

    QLineEdit* _ExeEditor = nullptr;
    QPushButton* _ExeBrowse = nullptr;
    QLabel* _StatusLabel = nullptr;
    QDialogButtonBox* _Buttons = nullptr;

    QComboBox* _GameComboBox = nullptr;
    QComboBox* _PlatformCombo = nullptr;
    QComboBox* _VendorCombo = nullptr;

    GameSnapshot* _ActiveSnapshot = nullptr;
};