#include "config.hpp"

#define LOG_TAG "[" PLUGIN_NAME "][timers-dialog]"
#include "fly_score_log.hpp"

#include "fly_score_timers_dialog.hpp"
#include "fly_score_state.hpp"
#include "fly_score_qt_helpers.hpp" // fly_parse_mmss_to_ms, fly_format_ms_mmss, fly_themed_icon, fly_style_icon_only_button

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QStyle>

FlyTimersDialog::FlyTimersDialog(const QString &dataDir, FlyState &state, QWidget *parent)
    : QDialog(parent)
    , dataDir_(dataDir)
    , st_(state)
{
    setWindowTitle(QStringLiteral("Edit timers"));
    setModal(true);
    resize(520, 400);

    buildUi();
    loadFromState();
}

void FlyTimersDialog::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(8);

    auto *box = new QGroupBox(QStringLiteral("Timers"), this);
    auto *boxLayout = new QVBoxLayout(box);
    boxLayout->setContentsMargins(8, 8, 8, 8);
    boxLayout->setSpacing(6);

    auto *hintLbl = new QLabel(
        QStringLiteral("Configure all timers.\n"
                       "Timer 1 is the main match timer and cannot be removed."),
        box);
    hintLbl->setWordWrap(true);
    boxLayout->addWidget(hintLbl);

    timersLayout_ = new QVBoxLayout();
    timersLayout_->setContentsMargins(0, 0, 0, 0);
    timersLayout_->setSpacing(4);
    boxLayout->addLayout(timersLayout_);

    addTimerBtn_ = new QPushButton(box);
    {
        const QIcon addIcon = fly_themed_icon(this, "list-add", QStyle::SP_FileDialogNewFolder);
        fly_style_icon_only_button(addTimerBtn_, addIcon,
                                   QStringLiteral("Add timer"));
    }
    addTimerBtn_->setCursor(Qt::PointingHandCursor);
    boxLayout->addWidget(addTimerBtn_, 0, Qt::AlignLeft);

    box->setLayout(boxLayout);
    root->addWidget(box);

    auto *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                        Qt::Horizontal, this);
    root->addWidget(btnBox);

    connect(addTimerBtn_, &QPushButton::clicked, this, &FlyTimersDialog::onAddTimer);
    connect(btnBox, &QDialogButtonBox::accepted, this, &FlyTimersDialog::onAccept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &FlyTimersDialog::reject);
}

FlyTimersDialog::Row FlyTimersDialog::addRow(const FlyTimer &tm, bool canRemove)
{
    Row r;

    auto *row = new QWidget(this);
    auto *lay = new QHBoxLayout(row);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(6);

    auto *labelEdit = new QLineEdit(row);
    labelEdit->setPlaceholderText(QStringLiteral("Timer label (e.g. First Half, Extra time)"));
    labelEdit->setText(tm.label);

    auto *timeEdit = new QLineEdit(row);
    timeEdit->setPlaceholderText(QStringLiteral("mm:ss"));
    timeEdit->setClearButtonEnabled(true);
    timeEdit->setMaxLength(8);
    timeEdit->setMinimumWidth(70);
    timeEdit->setText(fly_format_ms_mmss(tm.initial_ms > 0 ? tm.initial_ms : tm.remaining_ms));

    auto *modeCombo = new QComboBox(row);
    modeCombo->addItems({QStringLiteral("countdown"), QStringLiteral("countup")});
    if (tm.mode.isEmpty())
        modeCombo->setCurrentText(QStringLiteral("countdown"));
    else
        modeCombo->setCurrentText(tm.mode);

    auto *removeBtn = new QPushButton(row);
    {
        const QIcon delIcon = fly_themed_icon(this, "edit-delete", QStyle::SP_TrashIcon);
        fly_style_icon_only_button(removeBtn, delIcon,
                                   QStringLiteral("Remove this timer"));
    }
    removeBtn->setFixedWidth(28);
    removeBtn->setEnabled(canRemove);
    removeBtn->setVisible(canRemove);

    lay->addWidget(labelEdit, 1);
    lay->addWidget(new QLabel(QStringLiteral("Time:"), row));
    lay->addWidget(timeEdit);
    lay->addWidget(new QLabel(QStringLiteral("Type:"), row));
    lay->addWidget(modeCombo);
    lay->addWidget(removeBtn);

    row->setLayout(lay);
    timersLayout_->addWidget(row);

    r.row       = row;
    r.labelEdit = labelEdit;
    r.timeEdit  = timeEdit;
    r.modeCombo = modeCombo;
    r.remove    = removeBtn;

    if (canRemove) {
        connect(removeBtn, &QPushButton::clicked, this, [this, row]() {
            for (int i = 0; i < rows_.size(); ++i) {
                if (rows_[i].row == row) {
                    auto rr = rows_[i];
                    rows_.removeAt(i);
                    if (timersLayout_)
                        timersLayout_->removeWidget(rr.row);
                    rr.row->deleteLater();
                    break;
                }
            }
        });
    }

    return r;
}

void FlyTimersDialog::loadFromState()
{
    // Clear old rows
    for (auto &r : rows_) {
        if (timersLayout_ && r.row)
            timersLayout_->removeWidget(r.row);
        if (r.row)
            r.row->deleteLater();
    }
    rows_.clear();

    if (!timersLayout_)
        return;

    // Ensure at least one timer
    if (st_.timers.isEmpty()) {
        FlyTimer main;
        main.label        = QStringLiteral("First Half");
        main.mode         = QStringLiteral("countdown");
        main.running      = false;
        main.initial_ms   = 0;
        main.remaining_ms = 0;
        main.last_tick_ms = 0;
        st_.timers.push_back(main);
    }

    rows_.reserve(st_.timers.size());
    for (int i = 0; i < st_.timers.size(); ++i) {
        const auto &tm = st_.timers[i];
        const bool canRemove = (i > 0); // timers[0] cannot be removed
        Row r = addRow(tm, canRemove);
        rows_.push_back(r);
    }
}

void FlyTimersDialog::saveToState()
{
    st_.timers.clear();
    st_.timers.reserve(rows_.size());

    for (int i = 0; i < rows_.size(); ++i) {
        const auto &r = rows_[i];

        FlyTimer tm;
        tm.label = r.labelEdit ? r.labelEdit->text() : QString();

        QString mode = r.modeCombo ? r.modeCombo->currentText() : QStringLiteral("countdown");
        tm.mode = mode;

        qint64 ms = 0;
        if (r.timeEdit) {
            qint64 parsed = fly_parse_mmss_to_ms(r.timeEdit->text());
            ms = (parsed >= 0) ? parsed : 0;
        }

        tm.initial_ms   = ms;
        tm.remaining_ms = ms;
        tm.running      = false;
        tm.last_tick_ms = 0;

        st_.timers.push_back(tm);
    }

    // Ensure at least one timer
    if (st_.timers.isEmpty()) {
        FlyTimer main;
        main.label        = QStringLiteral("First Half");
        main.mode         = QStringLiteral("countdown");
        main.running      = false;
        main.initial_ms   = 0;
        main.remaining_ms = 0;
        main.last_tick_ms = 0;
        st_.timers.push_back(main);
    }

    // Persist to disk
    fly_state_save(dataDir_, st_);
}

void FlyTimersDialog::onAddTimer()
{
    FlyTimer tm;
    tm.label        = QString();
    tm.mode         = QStringLiteral("countdown");
    tm.running      = false;
    tm.initial_ms   = 0;
    tm.remaining_ms = 0;
    tm.last_tick_ms = 0;

    Row r = addRow(tm, true);
    rows_.push_back(r);
}

void FlyTimersDialog::onAccept()
{
    saveToState();
    accept();
}
