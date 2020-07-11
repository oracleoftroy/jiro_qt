#include "ui/MainWindow.hpp"

#include <QApplication>
#include "log.hpp"

int main(int argc, char *argv[])
{
	spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL));

	LOG_INFO("Initializing application");
	QApplication app(argc, argv);
	app.setApplicationName("Jiro");
	app.setOrganizationName("Jiro");

	LOG_INFO("Creating main window");
	ui::MainWindow window;
	window.show();

	LOG_INFO("Ready");
	return app.exec();
}
