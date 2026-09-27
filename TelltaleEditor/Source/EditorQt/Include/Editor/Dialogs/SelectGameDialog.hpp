#pragma once

#include <QDialog>
#include <Application.hpp>
#include <TelltaleEditor.hpp>

enum class SelectionStage
{
    SELECT_LOCATION,
    
};

class SelectGameDialog : public QDialog 
{

    Q_OBJECT

public:

    explicit SelectGameDialog(Application& app, QWidget* parent = nullptr);
    ~SelectGameDialog() override;

    const GameSnapshot& GetSnapshot() const { return _Snapshot; }

private slots:


private:

    SelectionStage _Stage = SelectionStage::SELECT_LOCATION;

    GameSnapshot _Snapshot;

    Application& _Application;

};