#pragma once

#include <QDialog>
#include <QVector>
#include <QString>

#include "fly_score_state.hpp"

class QVBoxLayout;
class QPushButton;
class QLineEdit;
class QComboBox;

class FlyTimersDialog : public QDialog {
    Q_OBJECT
public:
    FlyTimersDialog(const QString &dataDir, FlyState &state, QWidget *parent = nullptr);

private slots:
    void onAddTimer();
    void onAccept();

private:
    struct Row {
        QWidget    *row       = nullptr;
        QLineEdit  *labelEdit = nullptr;
        QLineEdit  *timeEdit  = nullptr;
        QComboBox  *modeCombo = nullptr;
        QPushButton *remove   = nullptr;
    };

    void buildUi();
    void loadFromState();
    void saveToState();
    Row  addRow(const FlyTimer &tm, bool canRemove);

private:
    QString  dataDir_;
    FlyState &st_;

    QVBoxLayout *timersLayout_ = nullptr;
    QPushButton *addTimerBtn_  = nullptr;
    QVector<Row> rows_;
};
