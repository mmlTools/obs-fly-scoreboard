// src/fly_score_plugin.cpp

#include "config.hpp"

#define LOG_TAG "[" PLUGIN_NAME "][plugin]"
#include "fly_score_log.hpp"

#include <obs-module.h>
#include <obs.h>

#include <QString>
#include <QDir>

#include "fly_score_paths.hpp"
#include "fly_score_server.hpp"
#include "fly_score_state.hpp"
#include "fly_score_dock.hpp"
#include "fly_score_const.hpp"

// ---------------------------------------------------------------------------
// OBS module entry
// ---------------------------------------------------------------------------

OBS_DECLARE_MODULE();
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

MODULE_EXPORT const char *obs_module_name(void)
{
	return PLUGIN_NAME;
}

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Fly Scoreboard — real-time sports/e-sports overlay with dock + HTTP server.";
}

bool obs_module_load(void)
{
	LOGI("Plugin loaded (version %s)", PLUGIN_VERSION);

	// Global data root: silently default on first run, then stick to it.
	const QString dataRoot    = fly_get_data_root(nullptr);
	const QString overlayRoot = QDir(dataRoot).filePath(QStringLiteral("overlay"));

	int port = fly_server_start(overlayRoot, 8089);
	if (port == 0)
		LOGW("Web server failed to bind");
	else
		LOGI("Web server listening on :%d (docRoot=%s)", port, overlayRoot.toUtf8().constData());

	// Create the main dock UI
	fly_create_dock();

	return true;
}

void obs_module_unload(void)
{
	LOGI("Plugin unloading...");

	// Tear down dock and web server
	fly_destroy_dock();
	fly_server_stop();

	LOGI("Plugin unloaded");
}
