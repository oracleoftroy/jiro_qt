#pragma once

#include <QMainWindow>
#include <QString>

#include "Actions.hpp"

class QTabWidget;
class QStackedLayout;

namespace ui
{
	class ProgressWidget;

	class MainWindow final : public QMainWindow
	{
		Q_OBJECT

	public:
		explicit MainWindow(QWidget *parent = nullptr);

	private:
		void closeEvent(QCloseEvent *event) override;
		void readSettings() noexcept;
		void addTab(const QString &archive) noexcept;

	private slots:
		void fileOpen() noexcept;
		void fileClose() noexcept;

	private:
		QTabWidget *tabs;
		QStackedLayout *mainLayout;
		ProgressWidget *progress;

		QString lastPath;
		Actions actions;
	};
}
