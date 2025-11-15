#pragma once
#include <QWidget>
#include <QString>

#include "fly_score_state.hpp"

class QTimer;
class QTimeEdit;
class QPushButton;
class QSpinBox;
class QLineEdit;
class QToolButton;
class QComboBox;
class QCheckBox;

class FlyScoreDock : public QWidget {
	Q_OBJECT
public:
	explicit FlyScoreDock(QWidget *parent = nullptr);
	bool init();

signals:
	void requestSave();

public slots:
	void bumpHomeScore(int delta);
	void bumpAwayScore(int delta);
	void bumpHomeRounds(int delta);
	void bumpAwayRounds(int delta);
	void toggleSwap();
	void toggleShow();

private slots:
	void onTick();
	void onStartStop();
	void onReset();
	void onTimeEdited();
	void onBrowseHomeLogo();
	void onBrowseAwayLogo();
	void onApply();
	void onClearTeamsAndReset();

private:
	void loadState();
	void saveState();
	void refreshUiFromState(bool onlyTimeIfRunning = false);
	void ensureTimerAccounting();

	QString copyLogoToOverlay(const QString &srcAbs, const QString &baseName);
	QString normalizedExtFromMime(const QString &path) const;

	bool deleteLogoIfExists(const QString &relPath);

private:
	QTimer *uiTimer_ = nullptr;
	QString dataDir_;
	FlyState st_;

	QLineEdit *time_label_ = nullptr;
	QComboBox *mode_ = nullptr;

	QLineEdit *time_ = nullptr;
	QPushButton *startStop_ = nullptr;
	QPushButton *reset_ = nullptr;

	QSpinBox *homeScore_ = nullptr;
	QSpinBox *awayScore_ = nullptr;
	QSpinBox *homeRounds_ = nullptr;
	QSpinBox *awayRounds_ = nullptr;

	QLineEdit *homeTitle_ = nullptr;
	QLineEdit *homeSub_ = nullptr;
	QLineEdit *homeLogo_ = nullptr;
	QToolButton *homeBrowse_ = nullptr;

	QLineEdit *awayTitle_ = nullptr;
	QLineEdit *awaySub_ = nullptr;
	QLineEdit *awayLogo_ = nullptr;
	QToolButton *awayBrowse_ = nullptr;

	QCheckBox *swapSides_ = nullptr;
	QCheckBox *showScoreboard_ = nullptr;

	QPushButton *applyBtn_ = nullptr;
};

void fly_create_dock();
void fly_destroy_dock();

FlyScoreDock *fly_get_dock();
