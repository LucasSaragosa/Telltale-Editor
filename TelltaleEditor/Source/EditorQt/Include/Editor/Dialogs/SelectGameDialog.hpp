#pragma once

#include <QDialog>

struct GameSnapshot;

class QLineEdit;
class QComboBox;
class QPushButton;
class QLabel;
class QDialogButtonBox;

class SelectGameDialog : public QDialog {
    Q_OBJECT

public:
    explicit SelectGameDialog(QWidget* parent = nullptr);
    ~SelectGameDialog() override;

    const GameSnapshot& GetSnapshot() const { return *m_gameSnapshot; }

private slots:
    void browseForExecutable();
    void onExecutableChanged(const QString& path);
    void onGameChanged(int index);
    void onAccept();

    void populateCombos();
    void detectGameFromPath(const QString& exePath);
    void updateAcceptState();
    bool validateInputs(QString* outError) const;
    void restoreFromSettings();

    QLineEdit* m_exeEdit = nullptr;
    QPushButton* m_exeBrowse = nullptr;
    QLabel* m_statusLabel = nullptr;
    QDialogButtonBox* m_buttons = nullptr;

    QComboBox* m_gameCombo = nullptr;
    QComboBox* m_platformCombo = nullptr;
    QComboBox* m_vendorCombo = nullptr;

    GameSnapshot* m_gameSnapshot = nullptr;
};