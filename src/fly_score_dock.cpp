#include "config.hpp"

#define LOG_TAG "[" PLUGIN_NAME "][dock]"
#include "fly_score_log.hpp"

#include "fly_score_dock.hpp"
#include "fly_score_state.hpp"
#include "fly_score_server.hpp"
#include "fly_score_settings_dialog.hpp"
#include "fly_score_const.hpp"
#include "fly_score_paths.hpp"
#include "fly_score_qt_helpers.hpp"
#include "fly_score_obs_helpers.hpp"
#include "fly_score_logo_helpers.hpp"
#include "fly_score_kofi_widget.hpp"
#include "fly_score_teams_dialog.hpp"
#include "fly_score_fields_dialog.hpp"
#include "fly_score_timers_dialog.hpp"
#include "fly_score_hotkeys_dialog.hpp"

#include <obs.h>
#ifdef ENABLE_FRONTEND_API
#include <obs-frontend-api.h>
#endif

#include <QAbstractButton>
#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QShortcut>
#include <QSize>
#include <QSizePolicy>
#include <QSpinBox>
#include <QSpacerItem>
#include <QStyle>
#include <QToolButton>

#include <algorithm>

// -----------------------------------------------------------------------------
// FlyScoreDock implementation
// -----------------------------------------------------------------------------

FlyScoreDock::FlyScoreDock(QWidget *parent) : QWidget(parent)
{
    // Make it comfortable to dock full-height
    QSizePolicy sp = sizePolicy();
    sp.setHorizontalPolicy(QSizePolicy::Preferred);
    sp.setVerticalPolicy(QSizePolicy::Expanding);
    setSizePolicy(sp);
}

bool FlyScoreDock::init()
{
    // Resolve global data root (auto-default on first run, no UI)
    dataDir_ = fly_get_data_root();
    loadState();

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(8);

    // ---------------------------------------------------------------------
    // Scoreboard box (single main panel)
    // ---------------------------------------------------------------------
    auto *scoreBox  = new QGroupBox(QStringLiteral("Scoreboard"), this);
    auto *scoreGrid = new QGridLayout();
    scoreGrid->setContentsMargins(8, 8, 8, 8);
    scoreGrid->setHorizontalSpacing(8);

    homeScore_ = new QSpinBox(scoreBox);
    homeScore_->setRange(0, 999);
    homeScore_->setValue(st_.home.score);

    awayScore_ = new QSpinBox(scoreBox);
    awayScore_->setRange(0, 999);
    awayScore_->setValue(st_.away.score);

    homeRounds_ = new QSpinBox(scoreBox);
    awayRounds_ = new QSpinBox(scoreBox);
    homeRounds_->setRange(0, 99);
    awayRounds_->setRange(0, 99);
    homeRounds_->setValue(st_.home.rounds);
    awayRounds_->setValue(st_.away.rounds);

    homeScore_->setMaximumWidth(80);
    awayScore_->setMaximumWidth(80);
    homeRounds_->setMaximumWidth(80);
    awayRounds_->setMaximumWidth(80);

    // Small helper for +/- icon buttons
    auto makeIconBtn = [this, scoreBox](const QString &themeName,
                                        QStyle::StandardPixmap fallback,
                                        const QString &tooltip) {
        auto *btn = new QPushButton(scoreBox);
        const QIcon icon = fly_themed_icon(this, themeName.toUtf8().constData(), fallback);
        fly_style_icon_only_button(btn, icon, tooltip);
        btn->setFixedWidth(26);
        return btn;
    };

    // Row: scores with +/- for Home / Guests
    int  srow       = 0;
    auto *homeLabel = new QLabel(QStringLiteral("Home:"), scoreBox);
    auto *guestLabel =
        new QLabel(QStringLiteral("Guests:"), scoreBox);

    auto *homeScoreMinus =
        makeIconBtn(QStringLiteral("list-remove"), QStyle::SP_ArrowDown, QStringLiteral("Home score -1"));
    auto *homeScorePlus =
        makeIconBtn(QStringLiteral("list-add"), QStyle::SP_ArrowUp, QStringLiteral("Home score +1"));
    auto *awayScoreMinus =
        makeIconBtn(QStringLiteral("list-remove"), QStyle::SP_ArrowDown, QStringLiteral("Guests score -1"));
    auto *awayScorePlus =
        makeIconBtn(QStringLiteral("list-add"), QStyle::SP_ArrowUp, QStringLiteral("Guests score +1"));

    int col = 0;
    scoreGrid->addWidget(homeLabel, srow, col++, Qt::AlignRight);
    scoreGrid->addWidget(homeScoreMinus, srow, col++);
    scoreGrid->addWidget(homeScore_, srow, col++);
    scoreGrid->addWidget(homeScorePlus, srow, col++);

    auto *scoreSpacer = new QSpacerItem(20, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    int   spacerCol   = col;
    scoreGrid->addItem(scoreSpacer, srow, col++);

    scoreGrid->addWidget(guestLabel, srow, col++, Qt::AlignRight);
    scoreGrid->addWidget(awayScoreMinus, srow, col++);
    scoreGrid->addWidget(awayScore_, srow, col++);
    scoreGrid->addWidget(awayScorePlus, srow, col++);

    scoreGrid->setColumnStretch(spacerCol, 1);

    // Row: rounds with +/- for Home / Guests
    srow++;
    auto *homeWinsLabel =
        new QLabel(QStringLiteral("Home Wins:"), scoreBox);
    auto *guestWinsLabel =
        new QLabel(QStringLiteral("Guests Wins:"), scoreBox);

    auto *homeRoundsMinus =
        makeIconBtn(QStringLiteral("list-remove"), QStyle::SP_ArrowDown, QStringLiteral("Home wins -1"));
    auto *homeRoundsPlus =
        makeIconBtn(QStringLiteral("list-add"), QStyle::SP_ArrowUp, QStringLiteral("Home wins +1"));
    auto *awayRoundsMinus =
        makeIconBtn(QStringLiteral("list-remove"), QStyle::SP_ArrowDown, QStringLiteral("Guests wins -1"));
    auto *awayRoundsPlus =
        makeIconBtn(QStringLiteral("list-add"), QStyle::SP_ArrowUp, QStringLiteral("Guests wins +1"));

    col = 0;
    scoreGrid->addWidget(homeWinsLabel, srow, col++, Qt::AlignRight);
    scoreGrid->addWidget(homeRoundsMinus, srow, col++);
    scoreGrid->addWidget(homeRounds_, srow, col++);
    scoreGrid->addWidget(homeRoundsPlus, srow, col++);

    auto *roundsSpacer = new QSpacerItem(20, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    int   roundsSpacerCol = col;
    scoreGrid->addItem(roundsSpacer, srow, col++);

    scoreGrid->addWidget(guestWinsLabel, srow, col++, Qt::AlignRight);
    scoreGrid->addWidget(awayRoundsMinus, srow, col++);
    scoreGrid->addWidget(awayRounds_, srow, col++);
    scoreGrid->addWidget(awayRoundsPlus, srow, col++);

    scoreGrid->setColumnStretch(roundsSpacerCol, 1);

    // Toggle row
    auto *toggleRow = new QHBoxLayout();
    swapSides_      = new QCheckBox(QStringLiteral("Swap Home ↔ Guests"), scoreBox);
    showScoreboard_ = new QCheckBox(QStringLiteral("Show Scoreboard"), scoreBox);
    showRounds_     = new QCheckBox(QStringLiteral("Show Wins"), scoreBox);

    toggleRow->addStretch(1);
    toggleRow->addWidget(swapSides_);
    toggleRow->addSpacing(12);
    toggleRow->addWidget(showScoreboard_);
    toggleRow->addSpacing(12);
    toggleRow->addWidget(showRounds_);
    toggleRow->addStretch(1);

    auto *scoreVBox = new QVBoxLayout(scoreBox);
    scoreVBox->setContentsMargins(8, 8, 8, 8);
    scoreVBox->setSpacing(6);
    scoreVBox->addLayout(scoreGrid);
    scoreVBox->addLayout(toggleRow);

    // ---------------------------------------------------------------------
    // Custom stats quick controls (inside Scoreboard panel)
    // ---------------------------------------------------------------------
    auto *customHeaderRow = new QHBoxLayout();
    auto *customTitleLbl  = new QLabel(QStringLiteral("Custom stats:"), scoreBox);
    editFieldsBtn_        = new QPushButton(scoreBox);
    {
        const QIcon editIcon =
            fly_themed_icon(this, "document-edit", QStyle::SP_FileDialogDetailedView);
        fly_style_icon_only_button(editFieldsBtn_, editIcon, QStringLiteral("Edit custom stats"));
    }
    editFieldsBtn_->setCursor(Qt::PointingHandCursor);
    customHeaderRow->addWidget(customTitleLbl);
    customHeaderRow->addStretch(1);
    customHeaderRow->addWidget(editFieldsBtn_);

    customFieldsLayout_ = new QVBoxLayout();
    customFieldsLayout_->setContentsMargins(0, 0, 0, 0);
    customFieldsLayout_->setSpacing(4);

    scoreVBox->addSpacing(8);
    scoreVBox->addLayout(customHeaderRow);
    scoreVBox->addLayout(customFieldsLayout_);

    // ---------------------------------------------------------------------
    // Timers quick controls (inside Scoreboard panel)
    // ---------------------------------------------------------------------
    auto *timersHeaderRow = new QHBoxLayout();
    auto *timersTitleLbl  = new QLabel(QStringLiteral("Timers:"), scoreBox);
    editTimersBtn_        = new QPushButton(scoreBox);
    {
        const QIcon editIcon =
            fly_themed_icon(this, "document-edit", QStyle::SP_FileDialogDetailedView);
        fly_style_icon_only_button(editTimersBtn_, editIcon, QStringLiteral("Edit timers"));
    }
    editTimersBtn_->setCursor(Qt::PointingHandCursor);
    timersHeaderRow->addWidget(timersTitleLbl);
    timersHeaderRow->addStretch(1);
    timersHeaderRow->addWidget(editTimersBtn_);

    timersLayout_ = new QVBoxLayout();
    timersLayout_->setContentsMargins(0, 0, 0, 0);
    timersLayout_->setSpacing(4);

    scoreVBox->addSpacing(8);
    scoreVBox->addLayout(timersHeaderRow);
    scoreVBox->addLayout(timersLayout_);

    scoreBox->setLayout(scoreVBox);

    // ---------------------------------------------------------------------
    // Bottom row buttons (footer, outside Scoreboard panel)
    // ---------------------------------------------------------------------
    auto *bottomRow = new QHBoxLayout();

    auto *refreshBrowserBtn = new QPushButton(this);
    {
        const QIcon refIcon = fly_themed_icon(this, "view-refresh", QStyle::SP_BrowserReload);
        fly_style_icon_only_button(
            refreshBrowserBtn, refIcon,
            QStringLiteral("Refresh ‘%1’").arg(kBrowserSourceName));
    }

    auto *addSourceBtn = new QPushButton(this);
    {
        const QIcon webIcon = QIcon::fromTheme(QStringLiteral("internet-web-browser"));
        const QIcon fbIcon  = this->style()->standardIcon(QStyle::SP_ComputerIcon);
        fly_style_icon_only_button(
            addSourceBtn, webIcon.isNull() ? fbIcon : webIcon,
            QStringLiteral("Add scoreboard Browser Source to current scene"));
    }

    auto *clearBtn = new QPushButton(this);
    {
        const QIcon delIcon = fly_themed_icon(this, "edit-clear", QStyle::SP_TrashIcon);
        fly_style_icon_only_button(
            clearBtn, delIcon,
            QStringLiteral("Clear teams, delete icons, reset settings"));
    }

    teamsBtn_   = new QPushButton(this);
    applyBtn_   = new QPushButton(this);
    auto *settingsBtn = new QPushButton(this);
    auto *hotkeysBtn  = new QPushButton(this);
    {
        const QIcon teamIcon = QIcon::fromTheme(QStringLiteral("user-group"));
        const QIcon teamFb   = this->style()->standardIcon(QStyle::SP_FileDialogInfoView);
        fly_style_icon_only_button(
            teamsBtn_,
            teamIcon.isNull() ? teamFb : teamIcon,
            QStringLiteral("Edit teams (names & logos)"));

        const QIcon saveIcon =
            fly_themed_icon(this, "document-save", QStyle::SP_DialogSaveButton);
        const QIcon settingsIcon = QIcon::fromTheme(QStringLiteral("preferences-system"));
        const QIcon settingsFb   =
            this->style()->standardIcon(QStyle::SP_FileDialogDetailedView);

        fly_style_icon_only_button(
            applyBtn_, saveIcon, QStringLiteral("Save / apply changes"));
        fly_style_icon_only_button(
            settingsBtn, settingsIcon.isNull() ? settingsFb : settingsIcon,
            QStringLiteral("Settings"));

        const QIcon hotkeyIcon =
            this->style()->standardIcon(QStyle::SP_DialogOpenButton);
        fly_style_icon_only_button(
            hotkeysBtn, hotkeyIcon,
            QStringLiteral("Configure Fly Scoreboard hotkeys"));
    }

    bottomRow->addWidget(refreshBrowserBtn);
    bottomRow->addWidget(addSourceBtn);
    bottomRow->addWidget(clearBtn);
    bottomRow->addStretch(1);
    bottomRow->addWidget(teamsBtn_);
    bottomRow->addWidget(applyBtn_);
    bottomRow->addWidget(settingsBtn);
    bottomRow->addWidget(hotkeysBtn);

    // ---------------------------------------------------------------------
    // Layout assembly: Scoreboard + footer + Ko-fi
    // ---------------------------------------------------------------------
    root->addWidget(scoreBox);
    root->addLayout(bottomRow);
    root->addStretch(1);
    root->addWidget(fly_create_kofi_card(this));

    // ---------------------------------------------------------------------
    // Connections for values -> state
    // ---------------------------------------------------------------------
    connect(homeScore_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int v) {
        st_.home.score = v;
        saveState();
    });
    connect(awayScore_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int v) {
        st_.away.score = v;
        saveState();
    });
    connect(homeRounds_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int v) {
        st_.home.rounds = v;
        saveState();
    });
    connect(awayRounds_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int v) {
        st_.away.rounds = v;
        saveState();
    });

    // +/- buttons for scores/rounds
    connect(homeScoreMinus, &QPushButton::clicked, this, [this]() { bumpHomeScore(-1); });
    connect(homeScorePlus,  &QPushButton::clicked, this, [this]() { bumpHomeScore(+1); });
    connect(awayScoreMinus, &QPushButton::clicked, this, [this]() { bumpAwayScore(-1); });
    connect(awayScorePlus,  &QPushButton::clicked, this, [this]() { bumpAwayScore(+1); });

    connect(homeRoundsMinus, &QPushButton::clicked, this, [this]() { bumpHomeRounds(-1); });
    connect(homeRoundsPlus,  &QPushButton::clicked, this, [this]() { bumpHomeRounds(+1); });
    connect(awayRoundsMinus, &QPushButton::clicked, this, [this]() { bumpAwayRounds(-1); });
    connect(awayRoundsPlus,  &QPushButton::clicked, this, [this]() { bumpAwayRounds(+1); });

    connect(swapSides_, &QCheckBox::toggled, this, [this](bool on) {
        st_.swap_sides = on;
        saveState();
    });
    connect(showScoreboard_, &QCheckBox::toggled, this, [this](bool on) {
        st_.show_scoreboard = on;
        saveState();
    });
    connect(showRounds_, &QCheckBox::toggled, this, [this](bool on) {
        st_.show_rounds = on;
        saveState();
    });

    connect(applyBtn_, &QPushButton::clicked, this, &FlyScoreDock::onApply);

    connect(refreshBrowserBtn, &QPushButton::clicked, this, []() {
        const bool ok =
            fly_refresh_browser_source_by_name(QString::fromUtf8(kBrowserSourceName));
        if (!ok)
            LOGW("Refresh failed for Browser Source: %s", kBrowserSourceName);
    });

    // Add browser source only when user explicitly clicks the button
    connect(addSourceBtn, &QPushButton::clicked, this, [this]() {
#ifdef ENABLE_FRONTEND_API
        obs_source_t *sceneSource = obs_frontend_get_current_scene();
        if (!sceneSource) {
            LOGW("No current scene when trying to add browser source");
            return;
        }

        obs_scene_t *scene = obs_scene_from_source(sceneSource);
        if (!scene) {
            LOGW("Current scene is not a scene");
            obs_source_release(sceneSource);
            return;
        }

        // Create a Browser source with the scoreboard URL
        QString url = QStringLiteral("http://127.0.0.1:%1/").arg(st_.server_port);

        obs_data_t *settings = obs_data_create();
        obs_data_set_string(settings, "url", url.toUtf8().constData());
        obs_data_set_bool(settings, "is_local_file", false);
        obs_data_set_int(settings, "width", 1920);
        obs_data_set_int(settings, "height", 1080);

        obs_source_t *browser =
            obs_source_create("browser_source", kBrowserSourceName, settings, nullptr);
        obs_data_release(settings);

        if (!browser) {
            LOGW("Failed to create browser source");
            obs_source_release(sceneSource);
            return;
        }

        obs_sceneitem_t *item = obs_scene_add(scene, browser);
        if (!item) {
            LOGW("Failed to add browser source to scene");
        }

        obs_source_release(browser);
        obs_source_release(sceneSource);
#else
        LOGW("Frontend API not enabled; cannot add browser source from dock.");
#endif
    });

    connect(settingsBtn, &QPushButton::clicked, this, [this]() {
        auto *dlg = new FlySettingsDialog(this);
        dlg->setAttribute(Qt::WA_DeleteOnClose, true);
        dlg->show();
    });

    connect(clearBtn, &QPushButton::clicked, this, &FlyScoreDock::onClearTeamsAndReset);

    connect(editFieldsBtn_, &QPushButton::clicked, this, &FlyScoreDock::onOpenCustomFieldsDialog);
    connect(editTimersBtn_, &QPushButton::clicked, this, &FlyScoreDock::onOpenTimersDialog);
    connect(teamsBtn_,      &QPushButton::clicked, this, &FlyScoreDock::onOpenTeamsDialog);

    connect(hotkeysBtn, &QPushButton::clicked, this, &FlyScoreDock::openHotkeysDialog);

    // Initial UI sync
    refreshUiFromState(false);

    // Initialise hotkey bindings (no shortcuts by default)
    hotkeyBindings_ = buildDefaultHotkeyBindings();
    applyHotkeyBindings(hotkeyBindings_);

    return true;
}

void FlyScoreDock::loadState()
{
    if (!fly_state_load(dataDir_, st_)) {
        st_ = fly_state_make_defaults();
        fly_state_save(dataDir_, st_);
    }

    // Ensure at least one timer (main timer)
    if (st_.timers.isEmpty()) {
        FlyTimer main;
        main.label        = QStringLiteral("First Half");
        main.mode         = QStringLiteral("countdown");
        main.running      = false;
        main.initial_ms   = 0;
        main.remaining_ms = 0;
        main.last_tick_ms = 0;
        st_.timers.push_back(main);
        fly_state_save(dataDir_, st_);
    } else if (st_.timers[0].mode.isEmpty()) {
        st_.timers[0].mode = QStringLiteral("countdown");
    }
}

void FlyScoreDock::saveState()
{
    fly_state_save(dataDir_, st_);
}

void FlyScoreDock::refreshUiFromState(bool onlyTimeIfRunning)
{
    Q_UNUSED(onlyTimeIfRunning);

    if (homeScore_ && !homeScore_->hasFocus() && homeScore_->value() != st_.home.score)
        homeScore_->setValue(st_.home.score);
    if (awayScore_ && !awayScore_->hasFocus() && awayScore_->value() != st_.away.score)
        awayScore_->setValue(st_.away.score);

    if (homeRounds_ && !homeRounds_->hasFocus() && homeRounds_->value() != st_.home.rounds)
        homeRounds_->setValue(st_.home.rounds);
    if (awayRounds_ && !awayRounds_->hasFocus() && awayRounds_->value() != st_.away.rounds)
        awayRounds_->setValue(st_.away.rounds);

    if (swapSides_ && swapSides_->isChecked() != st_.swap_sides)
        swapSides_->setChecked(st_.swap_sides);
    if (showScoreboard_ && showScoreboard_->isChecked() != st_.show_scoreboard)
        showScoreboard_->setChecked(st_.show_scoreboard);
    if (showRounds_ && showRounds_->isChecked() != st_.show_rounds)
        showRounds_->setChecked(st_.show_rounds);

    loadCustomFieldControlsFromState();
    loadTimerControlsFromState();
}

void FlyScoreDock::onApply()
{
    if (swapSides_)
        st_.swap_sides = swapSides_->isChecked();
    if (showScoreboard_)
        st_.show_scoreboard = showScoreboard_->isChecked();
    if (showRounds_)
        st_.show_rounds = showRounds_->isChecked();

    if (homeScore_)
        st_.home.score = homeScore_->value();
    if (awayScore_)
        st_.away.score = awayScore_->value();
    if (homeRounds_)
        st_.home.rounds = homeRounds_->value();
    if (awayRounds_)
        st_.away.rounds = awayRounds_->value();

    // Sync quick controls → state
    syncCustomFieldControlsToState();
    // timers quick controls modify state directly in button handlers

    refreshUiFromState(false);
}

void FlyScoreDock::onClearTeamsAndReset()
{
    auto rc = QMessageBox::question(
        this, QStringLiteral("Reset scoreboard"),
        QStringLiteral("Clear teams, delete uploaded icons, reset settings and custom stats?"));
    if (rc != QMessageBox::Yes)
        return;

    fly_delete_logo_if_exists(dataDir_, st_.home.logo);
    fly_delete_logo_if_exists(dataDir_, st_.away.logo);

    for (const auto &base : {QStringLiteral("home"), QStringLiteral("guest")})
        fly_clean_overlay_prefix(dataDir_, base);

    if (!fly_state_reset_defaults(dataDir_)) {
        LOGW("Failed to reset plugin.json to defaults");
    }

    clearAllCustomFieldRows();
    customFields_.clear();

    clearAllTimerRows();
    timers_.clear();

    loadState();
    refreshUiFromState(false);

    LOGI("Cleared teams, deleted icons, reset plugin.json, custom stats and timers");
}

static inline int clampi(int v, int lo, int hi)
{
    return std::max(lo, std::min(v, hi));
}

void FlyScoreDock::bumpHomeScore(int delta)
{
    int nv = clampi((homeScore_ ? homeScore_->value() : st_.home.score) + delta, 0, 999);
    if (homeScore_)
        homeScore_->setValue(nv);
    st_.home.score = nv;
    saveState();
}

void FlyScoreDock::bumpAwayScore(int delta)
{
    int nv = clampi((awayScore_ ? awayScore_->value() : st_.away.score) + delta, 0, 999);
    if (awayScore_)
        awayScore_->setValue(nv);
    st_.away.score = nv;
    saveState();
}

void FlyScoreDock::bumpHomeRounds(int delta)
{
    int nv = clampi((homeRounds_ ? homeRounds_->value() : st_.home.rounds) + delta, 0, 99);
    if (homeRounds_)
        homeRounds_->setValue(nv);
    st_.home.rounds = nv;
    saveState();
}

void FlyScoreDock::bumpAwayRounds(int delta)
{
    int nv = clampi((awayRounds_ ? awayRounds_->value() : st_.away.rounds) + delta, 0, 99);
    if (awayRounds_)
        awayRounds_->setValue(nv);
    st_.away.rounds = nv;
    saveState();
}

void FlyScoreDock::toggleSwap()
{
    bool nv = !(swapSides_ ? swapSides_->isChecked() : st_.swap_sides);
    if (swapSides_)
        swapSides_->setChecked(nv);
    st_.swap_sides = nv;
    saveState();
}

void FlyScoreDock::toggleShow()
{
    bool nv = !(showScoreboard_ ? showScoreboard_->isChecked() : st_.show_scoreboard);
    if (showScoreboard_)
        showScoreboard_->setChecked(nv);
    st_.show_scoreboard = nv;
    saveState();
}

// -----------------------------------------------------------------------------
// Custom fields quick controls
// -----------------------------------------------------------------------------

void FlyScoreDock::clearAllCustomFieldRows()
{
    for (auto &ui : customFields_) {
        if (customFieldsLayout_ && ui.row)
            customFieldsLayout_->removeWidget(ui.row);
        if (ui.row)
            ui.row->deleteLater();
    }
    customFields_.clear();
}

void FlyScoreDock::loadCustomFieldControlsFromState()
{
    clearAllCustomFieldRows();

    if (!customFieldsLayout_)
        return;

    customFields_.reserve(st_.custom_fields.size());

    for (int i = 0; i < st_.custom_fields.size(); ++i) {
        const auto &cf = st_.custom_fields[i];

        FlyCustomFieldUi ui;

        auto *row = new QWidget(this);
        auto *lay = new QHBoxLayout(row);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(6);

        auto *visibleCheck = new QCheckBox(row);
        visibleCheck->setChecked(cf.visible);
        visibleCheck->setToolTip(QStringLiteral("Show this field on overlay"));

        auto *labelLbl =
            new QLabel(cf.label.isEmpty() ? QStringLiteral("(unnamed)") : cf.label, row);
        labelLbl->setMinimumWidth(120);

        auto *homeSpin = new QSpinBox(row);
        homeSpin->setRange(0, 999);
        homeSpin->setValue(std::max(0, cf.home));
        homeSpin->setMaximumWidth(60);

        auto *awaySpin = new QSpinBox(row);
        awaySpin->setRange(0, 999);
        awaySpin->setValue(std::max(0, cf.away));
        awaySpin->setMaximumWidth(60);

        auto makeIconBtn = [this, row](const QString &themeName,
                                       QStyle::StandardPixmap fallback,
                                       const QString &tooltip) {
            auto *btn = new QPushButton(row);
            const QIcon icon = fly_themed_icon(this, themeName.toUtf8().constData(), fallback);
            fly_style_icon_only_button(btn, icon, tooltip);
            btn->setFixedWidth(26);
            return btn;
        };

        auto *minusHome =
            makeIconBtn(QStringLiteral("list-remove"), QStyle::SP_ArrowDown, QStringLiteral("Home -1"));
        auto *plusHome =
            makeIconBtn(QStringLiteral("list-add"), QStyle::SP_ArrowUp, QStringLiteral("Home +1"));
        auto *minusAway =
            makeIconBtn(QStringLiteral("list-remove"), QStyle::SP_ArrowDown, QStringLiteral("Guests -1"));
        auto *plusAway =
            makeIconBtn(QStringLiteral("list-add"), QStyle::SP_ArrowUp, QStringLiteral("Guests +1"));

        lay->addWidget(visibleCheck);
        lay->addWidget(labelLbl, 1);
        lay->addWidget(new QLabel(QStringLiteral("Home:"), row));
        lay->addWidget(homeSpin);
        lay->addWidget(minusHome);
        lay->addWidget(plusHome);
        lay->addSpacing(8);
        lay->addWidget(new QLabel(QStringLiteral("Guests:"), row));
        lay->addWidget(awaySpin);
        lay->addWidget(minusAway);
        lay->addWidget(plusAway);
        lay->addStretch(1);

        row->setLayout(lay);
        customFieldsLayout_->addWidget(row);

        ui.row         = row;
        ui.visibleCheck = visibleCheck;
        ui.labelLbl    = labelLbl;
        ui.homeSpin    = homeSpin;
        ui.awaySpin    = awaySpin;
        ui.minusHome   = minusHome;
        ui.plusHome    = plusHome;
        ui.minusAway   = minusAway;
        ui.plusAway    = plusAway;

        auto sync = [this]() {
            syncCustomFieldControlsToState();
        };

        connect(homeSpin, qOverload<int>(&QSpinBox::valueChanged),
                this, [sync](int) { sync(); });
        connect(awaySpin, qOverload<int>(&QSpinBox::valueChanged),
                this, [sync](int) { sync(); });
        connect(visibleCheck, &QCheckBox::toggled,
                this, [sync](bool) { sync(); });

        connect(minusHome, &QPushButton::clicked, this, [homeSpin, sync]() {
            homeSpin->setValue(std::max(0, homeSpin->value() - 1));
            sync();
        });
        connect(plusHome, &QPushButton::clicked, this, [homeSpin, sync]() {
            homeSpin->setValue(homeSpin->value() + 1);
            sync();
        });
        connect(minusAway, &QPushButton::clicked, this, [awaySpin, sync]() {
            awaySpin->setValue(std::max(0, awaySpin->value() - 1));
            sync();
        });
        connect(plusAway, &QPushButton::clicked, this, [awaySpin, sync]() {
            awaySpin->setValue(awaySpin->value() + 1);
            sync();
        });

        customFields_.push_back(ui);
    }
}

void FlyScoreDock::syncCustomFieldControlsToState()
{
    st_.custom_fields.clear();
    st_.custom_fields.reserve(customFields_.size());

    for (const auto &ui : customFields_) {
        FlyCustomField cf;
        cf.label   = ui.labelLbl ? ui.labelLbl->text() : QString();
        cf.home    = ui.homeSpin ? ui.homeSpin->value() : 0;
        cf.away    = ui.awaySpin ? ui.awaySpin->value() : 0;
        cf.visible = ui.visibleCheck ? ui.visibleCheck->isChecked() : true;
        st_.custom_fields.push_back(cf);
    }

    saveState();
}

// -----------------------------------------------------------------------------
// Timers quick controls
// -----------------------------------------------------------------------------

void FlyScoreDock::clearAllTimerRows()
{
    for (auto &ui : timers_) {
        if (timersLayout_ && ui.row)
            timersLayout_->removeWidget(ui.row);
        if (ui.row)
            ui.row->deleteLater();
    }
    timers_.clear();
}

void FlyScoreDock::loadTimerControlsFromState()
{
    clearAllTimerRows();

    if (!timersLayout_)
        return;

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

    timers_.reserve(st_.timers.size());

    for (int i = 0; i < st_.timers.size(); ++i) {
        const auto &tm = st_.timers[i];

        FlyTimerUi ui;

        auto *row = new QWidget(this);
        auto *lay = new QHBoxLayout(row);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(6);

        auto *labelLbl =
            new QLabel(tm.label.isEmpty() ? QStringLiteral("(unnamed)") : tm.label, row);
        labelLbl->setMinimumWidth(120);

        auto *timeEdit = new QLineEdit(row);
        timeEdit->setPlaceholderText(QStringLiteral("mm:ss"));
        timeEdit->setClearButtonEnabled(true);
        timeEdit->setMaxLength(8);
        timeEdit->setMinimumWidth(70);
        timeEdit->setText(fly_format_ms_mmss(tm.remaining_ms));

        auto *startStopBtn = new QPushButton(row);
        auto *resetBtn     = new QPushButton(row);

        const QIcon playIcon  =
            fly_themed_icon(this, "media-playback-start", QStyle::SP_MediaPlay);
        const QIcon pauseIcon =
            fly_themed_icon(this, "media-playback-pause", QStyle::SP_MediaPause);
        const QIcon resetIcon =
            fly_themed_icon(this, "view-refresh", QStyle::SP_BrowserReload);

        fly_style_icon_only_button(
            startStopBtn,
            tm.running ? pauseIcon : playIcon,
            tm.running ? QStringLiteral("Pause timer")
                       : QStringLiteral("Start timer"));
        fly_style_icon_only_button(
            resetBtn, resetIcon, QStringLiteral("Reset timer"));
        resetBtn->setFixedWidth(28);

        // Label on the left, everything else on the right
        lay->addWidget(labelLbl);
        lay->addStretch(1);
        lay->addWidget(timeEdit);
        lay->addWidget(startStopBtn);
        lay->addWidget(resetBtn);

        row->setLayout(lay);
        timersLayout_->addWidget(row);

        ui.row      = row;
        ui.labelLbl = labelLbl;
        ui.timeEdit = timeEdit;
        ui.startStop = startStopBtn;
        ui.reset    = resetBtn;

        // Edit time from dock (only when not running)
        connect(timeEdit, &QLineEdit::editingFinished,
                this, [this, i, timeEdit]() {
            if (i < 0 || i >= st_.timers.size())
                return;

            FlyTimer &tm = st_.timers[i];
            if (tm.running) {
                // revert if running
                timeEdit->setText(fly_format_ms_mmss(tm.remaining_ms));
                return;
            }

            qint64 ms = fly_parse_mmss_to_ms(timeEdit->text());
            if (ms < 0) {
                timeEdit->setText(fly_format_ms_mmss(tm.remaining_ms));
                return;
            }

            tm.initial_ms   = ms;
            tm.remaining_ms = ms;
            saveState();
            timeEdit->setText(fly_format_ms_mmss(tm.remaining_ms));
        });

        // Start/pause
        connect(startStopBtn, &QPushButton::clicked, this, [this, i]() {
            if (i < 0 || i >= st_.timers.size())
                return;

            FlyTimer &tm = st_.timers[i];

            if (!tm.running) {
                qint64 ms = tm.remaining_ms;
                if (tm.mode == QStringLiteral("countdown")) {
                    if (ms < 0)
                        ms = (tm.remaining_ms > 0 ? tm.remaining_ms : 0);
                    tm.initial_ms   = ms;
                    tm.remaining_ms = ms;
                } else {
                    if (ms < 0)
                        ms = 0;
                    tm.initial_ms   = ms;
                    tm.remaining_ms = ms;
                }
                tm.last_tick_ms = fly_now_ms();
                tm.running      = true;
            } else {
                tm.running = false;
            }

            saveState();
            refreshUiFromState(false);
        });

        // Reset
        connect(resetBtn, &QPushButton::clicked, this, [this, i]() {
            if (i < 0 || i >= st_.timers.size())
                return;

            FlyTimer &tm = st_.timers[i];

            qint64 ms = tm.initial_ms;
            if (tm.mode == QStringLiteral("countdown")) {
                if (ms < 0)
                    ms = 0;
            } else {
                if (ms < 0)
                    ms = 0;
            }

            tm.remaining_ms = ms;
            tm.running      = false;
            tm.last_tick_ms = 0;

            saveState();
            refreshUiFromState(false);
        });

        timers_.push_back(ui);
    }
}

// -----------------------------------------------------------------------------
// Hotkeys – dialog-driven, plugin-local (QShortcut)
// -----------------------------------------------------------------------------

QVector<FlyHotkeyBinding> FlyScoreDock::buildDefaultHotkeyBindings() const
{
    QVector<FlyHotkeyBinding> v;
    v.reserve(10);

    // No default key sequences; user can assign them in the dialog.
    v.push_back({"home_score_inc",  tr("Home Score +1"),          QKeySequence()});
    v.push_back({"home_score_dec",  tr("Home Score -1"),          QKeySequence()});
    v.push_back({"away_score_inc",  tr("Guests Score +1"),        QKeySequence()});
    v.push_back({"away_score_dec",  tr("Guests Score -1"),        QKeySequence()});

    v.push_back({"home_rounds_inc", tr("Home Wins +1"),           QKeySequence()});
    v.push_back({"home_rounds_dec", tr("Home Wins -1"),           QKeySequence()});
    v.push_back({"away_rounds_inc", tr("Guests Wins +1"),         QKeySequence()});
    v.push_back({"away_rounds_dec", tr("Guests Wins -1"),         QKeySequence()});

    v.push_back({"toggle_swap",     tr("Swap Home ↔ Guests"),     QKeySequence()});
    v.push_back({"toggle_show",     tr("Show / Hide Scoreboard"), QKeySequence()});

    // In future you can append dynamic "field_*" and "timer_*" actions here.

    return v;
}

void FlyScoreDock::clearAllShortcuts()
{
    for (auto *sc : shortcuts_) {
        if (sc)
            sc->deleteLater();
    }
    shortcuts_.clear();
}

void FlyScoreDock::applyHotkeyBindings(const QVector<FlyHotkeyBinding> &bindings)
{
    clearAllShortcuts();
    hotkeyBindings_ = bindings;

    for (const auto &b : bindings) {
        if (b.sequence.isEmpty())
            continue;

        auto *sc = new QShortcut(b.sequence, this);
        sc->setContext(Qt::WidgetWithChildrenShortcut);
        shortcuts_.push_back(sc);

        const QString &id = b.actionId;

        if (id == QLatin1String("home_score_inc")) {
            connect(sc, &QShortcut::activated, this, [this]() { bumpHomeScore(+1); });
        } else if (id == QLatin1String("home_score_dec")) {
            connect(sc, &QShortcut::activated, this, [this]() { bumpHomeScore(-1); });
        } else if (id == QLatin1String("away_score_inc")) {
            connect(sc, &QShortcut::activated, this, [this]() { bumpAwayScore(+1); });
        } else if (id == QLatin1String("away_score_dec")) {
            connect(sc, &QShortcut::activated, this, [this]() { bumpAwayScore(-1); });
        } else if (id == QLatin1String("home_rounds_inc")) {
            connect(sc, &QShortcut::activated, this, [this]() { bumpHomeRounds(+1); });
        } else if (id == QLatin1String("home_rounds_dec")) {
            connect(sc, &QShortcut::activated, this, [this]() { bumpHomeRounds(-1); });
        } else if (id == QLatin1String("away_rounds_inc")) {
            connect(sc, &QShortcut::activated, this, [this]() { bumpAwayRounds(+1); });
        } else if (id == QLatin1String("away_rounds_dec")) {
            connect(sc, &QShortcut::activated, this, [this]() { bumpAwayRounds(-1); });
        } else if (id == QLatin1String("toggle_swap")) {
            connect(sc, &QShortcut::activated, this, [this]() { toggleSwap(); });
        } else if (id == QLatin1String("toggle_show")) {
            connect(sc, &QShortcut::activated, this, [this]() { toggleShow(); });
        }
        // Future: handle "field_*" and "timer_*" IDs here.
    }
}

void FlyScoreDock::openHotkeysDialog()
{
    QVector<FlyHotkeyBinding> initial =
        hotkeyBindings_.isEmpty() ? buildDefaultHotkeyBindings() : hotkeyBindings_;

    auto *dlg = new FlyHotkeysDialog(initial, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);

    connect(dlg, &FlyHotkeysDialog::bindingsChanged,
            this, [this](const QVector<FlyHotkeyBinding> &b) {
        applyHotkeyBindings(b);
        // TODO: persist b into FlyState and saveState() if you want them to survive restarts.
    });

    dlg->show();
}

// -----------------------------------------------------------------------------
// Dialogs
// -----------------------------------------------------------------------------

void FlyScoreDock::onOpenCustomFieldsDialog()
{
    FlyFieldsDialog dlg(dataDir_, st_, this);
    dlg.exec();

    loadState();
    refreshUiFromState(false);
}

void FlyScoreDock::onOpenTimersDialog()
{
    FlyTimersDialog dlg(dataDir_, st_, this);
    dlg.exec();

    loadState();
    refreshUiFromState(false);
}

void FlyScoreDock::onOpenTeamsDialog()
{
    FlyTeamsDialog dlg(dataDir_, st_, this);
    dlg.exec();

    // Reload state from disk in case other fields were touched
    loadState();
    refreshUiFromState(false);
}

// -----------------------------------------------------------------------------
// Dock registration with OBS frontend
// -----------------------------------------------------------------------------

static QWidget *g_dockContent = nullptr;

void fly_create_dock()
{
    if (g_dockContent)
        return;

    auto *panel = new FlyScoreDock(nullptr);
    panel->init();

#if defined(HAVE_OBS_DOCK_BY_ID)
    obs_frontend_add_dock_by_id(kFlyDockId, kFlyDockTitle, panel);
#else
    obs_frontend_add_dock(panel);
#endif

    g_dockContent = panel;
}

void fly_destroy_dock()
{
    if (!g_dockContent)
        return;

#if defined(HAVE_OBS_DOCK_BY_ID)
    obs_frontend_remove_dock(kFlyDockId);
#else
    obs_frontend_remove_dock(g_dockContent);
#endif

    g_dockContent = nullptr;
}

FlyScoreDock *fly_get_dock()
{
    return qobject_cast<FlyScoreDock *>(g_dockContent);
}
