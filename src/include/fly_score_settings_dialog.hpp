#pragma once
#include <QDialog>
#include <QString>

class QLabel;
class QSpinBox;
class QPushButton;
class QTimer;

class FlySettingsDialog : public QDialog {
	Q_OBJECT
public:
	explicit FlySettingsDialog(const QString &baseDir, QWidget *parent = nullptr);
	~FlySettingsDialog() override;

private slots:
	void onPollHealth();
	void onRestartServer();
	void onOpenOverlay();
	void onOpenHotkeysHelp();

private:
	void setStatusUi(bool ok, int port);
	bool healthOkHttp(int port, int timeout_ms = 400);

private:
	QString baseDir_;
	QLabel *statusDot_ = nullptr;
	QLabel *statusText_ = nullptr;
	QSpinBox *portSpin_ = nullptr;
	QPushButton *restartBtn_ = nullptr;
	QPushButton *openFolderBtn_ = nullptr;
	QPushButton *hotkeysBtn_ = nullptr;
	QTimer *statusTimer_ = nullptr;
};
