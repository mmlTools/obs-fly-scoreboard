#pragma once

#include <QDialog>
#include <QKeySequence>
#include <QVector>

class QTableWidget;
class QAbstractButton;

struct FlyHotkeyBinding {
    QString      actionId;   // internal stable ID (e.g. "home_score_inc")
    QString      label;      // user-visible label
    QKeySequence sequence;   // may be empty (no shortcut)
};

class FlyHotkeysDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FlyHotkeysDialog(const QVector<FlyHotkeyBinding> &initial,
                              QWidget *parent = nullptr);

    QVector<FlyHotkeyBinding> bindings() const;

signals:
    void bindingsChanged(const QVector<FlyHotkeyBinding> &bindings);

private slots:
    void onAccept();

private:
    void buildUi(const QVector<FlyHotkeyBinding> &initial);

    QTableWidget   *table_      = nullptr;
    QAbstractButton *btnResetAll_ = nullptr;
};
