#include "seed_defaults.hpp"
#include "fly_score_state.hpp"

#include <obs-module.h>
#include <util/platform.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#ifdef ENABLE_EMBEDDED_DEFAULTS
#include "embedded_assets.hpp"
#endif

static QString moduleBaseDirFromConfigFile()
{
	char *p = obs_module_config_path("plugin.json");
	QString filePath = p ? QString::fromUtf8(p) : QString();
	if (p)
		bfree(p);
	return filePath.isEmpty() ? QDir::homePath() : QFileInfo(filePath).absolutePath();
}

static void write_text_file(const QString &path, const QByteArray &data)
{
	QDir().mkpath(QFileInfo(path).absolutePath());
	QFile f(path);
	if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
		f.write(data);
}

static void copy_if_missing(const QString &src, const QString &dstDir)
{
	const QString dst = dstDir + "/" + QFileInfo(src).fileName();
	if (!QFile::exists(dst) && QFile::exists(src)) {
		QDir().mkpath(dstDir);
		QFile::copy(src, dst);
	}
}

QString seed_defaults_if_needed()
{
	const QString base = QDir::cleanPath(moduleBaseDirFromConfigFile());
	const QString overlay = base + "/overlay";

#ifdef ENABLE_EMBEDDED_DEFAULTS
	using namespace fly_score_embedded;
	QDir().mkpath(overlay);
#if defined(HAVE_index_html)
	if (!QFile::exists(overlay + "/index.html"))
		write_text_file(overlay + "/index.html", QByteArray(index_html));
#endif
#if defined(HAVE_style_css)
	if (!QFile::exists(overlay + "/style.css"))
		write_text_file(overlay + "/style.css", QByteArray(style_css));
#endif
#if defined(HAVE_script_js)
	if (!QFile::exists(overlay + "/script.js"))
		write_text_file(overlay + "/script.js", QByteArray(script_js));
#endif
#else
	const QString overlaySrc = QDir(QCoreApplication::applicationDirPath() + "/../../data/overlay").absolutePath();
	QDir().mkpath(overlay);
	copy_if_missing(overlaySrc + "/index.html", overlay);
	copy_if_missing(overlaySrc + "/style.css", overlay);
	copy_if_missing(overlaySrc + "/script.js", overlay);
#endif

	FlyState st;
	if (!fly_state_load(base, st)) {
		st.server_port = 8089;
		st.time_label = "First Half";
		st.home.title = "Home";
		st.away.title = "Guests";
		st.timer.mode = "countdown";
		st.timer.running = false;
		fly_state_save(base, st);
	}

	return base;
}
