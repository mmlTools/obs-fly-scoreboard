#include "fly_score_hotkeys_dialog.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QKeySequenceEdit>
#include <QPushButton>
#include <QLabel>

FlyHotkeysDialog::FlyHotkeysDialog(const QVector<FlyHotkeyBinding> &initial,
                                   QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Fly Scoreboard – Hotkeys"));
    setModal(true);
    resize(520, 400);

    buildUi(initial);
}

void FlyHotkeysDialog::buildUi(const QVector<FlyHotkeyBinding> &initial)
{
    auto *root = new QVBoxLayout(this);

    auto *hint = new QLabel(
        tr("Assign keyboard shortcuts for Fly Scoreboard actions.\n"
           "Leave a shortcut empty to disable it."),
        this);
    hint->setWordWrap(true);
    root->addWidget(hint);

    table_ = new QTableWidget(this);
    table_->setColumnCount(3);
    QStringList headers;
    headers << tr("Action") << tr("Shortcut") << "";
    table_->setHorizontalHeaderLabels(headers);
    table_->horizontalHeader()->setStretchLastSection(false);
    table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    table_->verticalHeader()->setVisible(false);
    table_->setShowGrid(true);
    table_->setSelectionMode(QAbstractItemView::NoSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);

    table_->setRowCount(initial.size());
    for (int row = 0; row < initial.size(); ++row) {
        const auto &b = initial[row];

        auto *item = new QTableWidgetItem(b.label);
        item->setData(Qt::UserRole, b.actionId);
        table_->setItem(row, 0, item);

        auto *seqEdit = new QKeySequenceEdit(b.sequence, table_);
        table_->setCellWidget(row, 1, seqEdit);

        auto *clearBtn = new QPushButton(tr("Clear"), table_);
        clearBtn->setAutoDefault(false);
        clearBtn->setDefault(false);
        table_->setCellWidget(row, 2, clearBtn);

        QObject::connect(clearBtn, &QPushButton::clicked, this, [seqEdit]() {
            seqEdit->clear();
        });
    }

    root->addWidget(table_);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    btnResetAll_ = buttons->addButton(tr("Reset to Defaults"),
                                      QDialogButtonBox::ResetRole);

    connect(buttons, &QDialogButtonBox::accepted,
            this, &FlyHotkeysDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected,
            this, &FlyHotkeysDialog::reject);

    connect(btnResetAll_, &QAbstractButton::clicked, this, [this]() {
        for (int row = 0; row < table_->rowCount(); ++row) {
            if (auto *w = table_->cellWidget(row, 1)) {
                if (auto *seqEdit = qobject_cast<QKeySequenceEdit *>(w))
                    seqEdit->clear();
            }
        }
    });

    root->addWidget(buttons);
}

QVector<FlyHotkeyBinding> FlyHotkeysDialog::bindings() const
{
    QVector<FlyHotkeyBinding> out;
    out.reserve(table_->rowCount());

    for (int row = 0; row < table_->rowCount(); ++row) {
        auto *item = table_->item(row, 0);
        if (!item)
            continue;

        QString id    = item->data(Qt::UserRole).toString();
        QString label = item->text();

        QKeySequence seq;
        if (auto *w = table_->cellWidget(row, 1)) {
            if (auto *seqEdit = qobject_cast<QKeySequenceEdit *>(w))
                seq = seqEdit->keySequence();
        }

        out.push_back(FlyHotkeyBinding{id, label, seq});
    }

    return out;
}

void FlyHotkeysDialog::onAccept()
{
    auto current = bindings();
    emit bindingsChanged(current);
    accept();
}
