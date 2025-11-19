#pragma once

#include <QDialog>
#include <QVector>
#include <QString>

#include "fly_score_state.hpp"

class QVBoxLayout;
class QPushButton;
class QLineEdit;
class QSpinBox;

class FlyFieldsDialog : public QDialog {
    Q_OBJECT
public:
    FlyFieldsDialog(const QString &dataDir, FlyState &state, QWidget *parent = nullptr);

private slots:
    void onAddField();
    void onAccept();

private:
    struct Row {
        QWidget    *row       = nullptr;
        QLineEdit  *labelEdit = nullptr;
        QSpinBox   *homeSpin  = nullptr;
        QSpinBox   *awaySpin  = nullptr;
        QPushButton *remove   = nullptr;
    };

    void buildUi();
    void loadFromState();
    void saveToState();
    Row  addRow(const FlyCustomField &cf);

private:
    QString  dataDir_;
    FlyState &st_;

    QVBoxLayout *fieldsLayout_ = nullptr;
    QPushButton *addFieldBtn_  = nullptr;
    QVector<Row> rows_;

    // Preserve visibility flags (controlled from dock)
    QVector<bool> vis_;
};
