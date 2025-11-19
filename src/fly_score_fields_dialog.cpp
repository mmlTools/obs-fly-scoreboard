#include "config.hpp"

#define LOG_TAG "[" PLUGIN_NAME "][fields-dialog]"
#include "fly_score_log.hpp"

#include "fly_score_fields_dialog.hpp"
#include "fly_score_state.hpp"
#include "fly_score_qt_helpers.hpp" // fly_themed_icon, fly_style_icon_only_button

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QStyle>

FlyFieldsDialog::FlyFieldsDialog(const QString &dataDir, FlyState &state, QWidget *parent)
    : QDialog(parent)
    , dataDir_(dataDir)
    , st_(state)
{
    setWindowTitle(QStringLiteral("Edit custom stats"));
    setModal(true);
    resize(520, 400);

    buildUi();
    loadFromState();
}

void FlyFieldsDialog::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(8);

    auto *box = new QGroupBox(QStringLiteral("Custom stats (corners, penalties, etc.)"), this);
    auto *boxLayout = new QVBoxLayout(box);
    boxLayout->setContentsMargins(8, 8, 8, 8);
    boxLayout->setSpacing(6);

    auto *hintLbl = new QLabel(
        QStringLiteral("Configure all your custom numeric fields.\n"
                       "Example labels: Corners, Penalties, Shots on goal, etc."),
        box);
    hintLbl->setWordWrap(true);
    boxLayout->addWidget(hintLbl);

    fieldsLayout_ = new QVBoxLayout();
    fieldsLayout_->setContentsMargins(0, 0, 0, 0);
    fieldsLayout_->setSpacing(4);
    boxLayout->addLayout(fieldsLayout_);

    addFieldBtn_ = new QPushButton(box);
    {
        const QIcon addIcon = fly_themed_icon(this, "list-add", QStyle::SP_FileDialogNewFolder);
        fly_style_icon_only_button(addFieldBtn_, addIcon,
                                   QStringLiteral("Add custom field"));
    }
    addFieldBtn_->setCursor(Qt::PointingHandCursor);
    boxLayout->addWidget(addFieldBtn_, 0, Qt::AlignLeft);

    box->setLayout(boxLayout);
    root->addWidget(box);

    auto *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                        Qt::Horizontal, this);
    root->addWidget(btnBox);

    connect(addFieldBtn_, &QPushButton::clicked, this, &FlyFieldsDialog::onAddField);
    connect(btnBox, &QDialogButtonBox::accepted, this, &FlyFieldsDialog::onAccept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &FlyFieldsDialog::reject);
}

FlyFieldsDialog::Row FlyFieldsDialog::addRow(const FlyCustomField &cf)
{
    Row r;

    auto *row = new QWidget(this);
    auto *lay = new QHBoxLayout(row);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(6);

    auto *labelEdit = new QLineEdit(row);
    labelEdit->setPlaceholderText(QStringLiteral("Label (e.g. Corners)"));
    labelEdit->setText(cf.label);

    auto *homeSpin = new QSpinBox(row);
    homeSpin->setRange(0, 999);
    homeSpin->setValue(std::max(0, cf.home));
    homeSpin->setMaximumWidth(70);

    auto *awaySpin = new QSpinBox(row);
    awaySpin->setRange(0, 999);
    awaySpin->setValue(std::max(0, cf.away));
    awaySpin->setMaximumWidth(70);

    auto *removeBtn = new QPushButton(row);
    {
        const QIcon delIcon = fly_themed_icon(this, "edit-delete", QStyle::SP_TrashIcon);
        fly_style_icon_only_button(removeBtn, delIcon,
                                   QStringLiteral("Remove this field"));
    }
    removeBtn->setFixedWidth(28);

    lay->addWidget(labelEdit, 1);
    lay->addWidget(new QLabel(QStringLiteral("Home:"), row));
    lay->addWidget(homeSpin);
    lay->addWidget(new QLabel(QStringLiteral("Guests:"), row));
    lay->addWidget(awaySpin);
    lay->addWidget(removeBtn);

    row->setLayout(lay);
    fieldsLayout_->addWidget(row);

    r.row       = row;
    r.labelEdit = labelEdit;
    r.homeSpin  = homeSpin;
    r.awaySpin  = awaySpin;
    r.remove    = removeBtn;

    connect(removeBtn, &QPushButton::clicked, this, [this, row]() {
        for (int i = 0; i < rows_.size(); ++i) {
            if (rows_[i].row == row) {
                auto rr = rows_[i];
                rows_.removeAt(i);
                if (i >= 0 && i < vis_.size())
                    vis_.removeAt(i);
                if (fieldsLayout_)
                    fieldsLayout_->removeWidget(rr.row);
                rr.row->deleteLater();
                break;
            }
        }
    });

    return r;
}

void FlyFieldsDialog::loadFromState()
{
    // Clear old
    for (auto &r : rows_) {
        if (fieldsLayout_ && r.row)
            fieldsLayout_->removeWidget(r.row);
        if (r.row)
            r.row->deleteLater();
    }
    rows_.clear();
    vis_.clear();

    if (!fieldsLayout_)
        return;

    rows_.reserve(st_.custom_fields.size());
    vis_.reserve(st_.custom_fields.size());

    for (const auto &cf : st_.custom_fields) {
        Row r = addRow(cf);
        rows_.push_back(r);
        vis_.push_back(cf.visible); // preserve visibility from state (dock controls it)
    }
}

void FlyFieldsDialog::saveToState()
{
    st_.custom_fields.clear();
    st_.custom_fields.reserve(rows_.size());

    for (int i = 0; i < rows_.size(); ++i) {
        const auto &r = rows_[i];

        FlyCustomField cf;
        cf.label   = r.labelEdit ? r.labelEdit->text() : QString();
        cf.home    = r.homeSpin ? r.homeSpin->value() : 0;
        cf.away    = r.awaySpin ? r.awaySpin->value() : 0;
        cf.visible = (i >= 0 && i < vis_.size()) ? vis_[i] : true;

        st_.custom_fields.push_back(cf);
    }

    // Persist to disk
    fly_state_save(dataDir_, st_);
}

void FlyFieldsDialog::onAddField()
{
    FlyCustomField cf;
    cf.label   = QString();
    cf.home    = 0;
    cf.away    = 0;
    cf.visible = true;

    Row r = addRow(cf);
    rows_.push_back(r);
    vis_.push_back(true);
}

void FlyFieldsDialog::onAccept()
{
    saveToState();
    accept();
}
