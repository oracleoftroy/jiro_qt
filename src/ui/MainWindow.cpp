#include "MainWindow.hpp"

#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMenuBar>
#include <QSettings>
#include <QStackedLayout>
#include <QStatusBar>
#include <QTabWidget>

#include <tuple>
#include <utility>

#include "../log.hpp"
#include "ImageView.hpp"
#include "ProgressWidget.hpp"

namespace ui
{
	static const auto tab_style =
		R"(QTabWidget::pane { /* The tab widget frame */
	border-top: 1px solid #C2C7CB;
}

QTabWidget::tab-bar {
	left: 5px; /* move to the right by 5px */
}

/* Style the tab using the tab sub-control. Note that it reads QTabBar _not_ QTabWidget */
QTabBar::tab {
	background: qlineargradient(
		x1: 0, y1: 0, x2: 0, y2: 1,
		stop: 0 #E1E1E1, stop: 0.4 #DDDDDD,
		stop: 0.5 #D8D8D8, stop: 1.0 #D3D3D3);
	border: 1px solid #C4C4C3;
	border-bottom-color: #C2C7CB; /* same as the pane color */
	border-top-left-radius: 4px;
	border-top-right-radius: 4px;
	min-width: 8ex;
	padding: 4px 8px;
}

QTabBar::tab:selected, QTabBar::tab:hover {
	background: qlineargradient(
		x1: 0, y1: 0, x2: 0, y2: 1,
		stop: 0 #fafafa, stop: 0.4 #f4f4f4,
		stop: 0.5 #e7e7e7, stop: 1.0 #fafafa);
}

QTabBar::tab:selected {
	border-color: #9B9B9B;
	border-bottom-color: #C2C7CB; /* same as pane color */
}

QTabBar::tab:!selected {
	margin-top: 2px; /* make non-selected tabs look smaller */
}

/* make use of negative margins for overlapping tabs */
QTabBar::tab:selected {
	/* expand/overlap to the left and right by 4px */
	margin-left: -4px;
	margin-right: -4px;
}

QTabBar::tab:first:selected {
	margin-left: 0; /* the first selected tab has nothing to overlap with on the left */
}

QTabBar::tab:last:selected {
	margin-right: 0; /* the last selected tab has nothing to overlap with on the right */
}

QTabBar::tab:only-one {
	margin: 0; /* if there is only one tab, we don't want overlapping margins */
}

QTabBar:focus {
	outline: none;
}

QTabBar::tab:focus:selected {
	border: 1px dotted #000;
}
)";

	MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
	{
		QMenuBar *mainMenu = new QMenuBar;
		auto fileMenu = mainMenu->addMenu(tr("&File"));

		actions.open = fileMenu->addAction(tr("&Open"));
		actions.open->setShortcut(QKeySequence::Open);
		connect(actions.open, &QAction::triggered, this, &MainWindow::fileOpen);

		actions.close = fileMenu->addAction(tr("&Close"));
		actions.close->setShortcut(QKeySequence::Close);
		connect(actions.close, &QAction::triggered, this, &MainWindow::fileClose);

		fileMenu->addSeparator();

		actions.quit = fileMenu->addAction(tr("E&xit"));
		actions.quit->setShortcut(QKeySequence::Quit);
		actions.quit->setMenuRole(QAction::QuitRole);
		connect(actions.quit, &QAction::triggered, this, &MainWindow::close);

		auto viewMenu = mainMenu->addMenu(tr("&View"));

		actions.fitH = viewMenu->addAction(tr("Fit images &horizontally"));
		actions.fitH->setCheckable(true);
		actions.fitH->setShortcut({QKeySequence{Qt::CTRL + Qt::Key_1}});

		actions.fitV = viewMenu->addAction(tr("Fit images &vertically"));
		actions.fitV->setCheckable(true);
		actions.fitV->setShortcut({QKeySequence{Qt::CTRL + Qt::Key_2}});

		viewMenu->addSeparator();

		actions.fullscreen = viewMenu->addAction(tr("&Full screen"));
		actions.fullscreen->setCheckable(true);
		actions.fullscreen->setShortcut(QKeySequence::FullScreen);
		connect(actions.fullscreen, &QAction::toggled, this, [this](bool fullscreen) {
			auto state = windowState();
			if (fullscreen)
				state |= Qt::WindowFullScreen;
			else
				state &= ~Qt::WindowFullScreen;

			setWindowState(state);
		});

		// auto helpMenu = mainMenu->addMenu(tr("&Help"));
		// auto a = helpMenu->addAction(tr("&About"));
		// a->setMenuRole(QAction::AboutRole);

		setMenuBar(mainMenu);

		mainLayout = new QStackedLayout();
		auto emptyLabel = new QLabel(tr("No files selected"));
		emptyLabel->setAlignment(Qt::AlignCenter);
		emptyLabel->setScaledContents(true);

		mainLayout->addWidget(emptyLabel);
		mainLayout->addWidget(tabs = new QTabWidget);
		mainLayout->setCurrentIndex(0);

		auto centralWidget = new QWidget();

		centralWidget->setLayout(mainLayout);
		setCentralWidget(centralWidget);

		connect(tabs, &QTabWidget::currentChanged, this, [this](int index) {
			auto widget = tabs->widget(index);
			auto view = qobject_cast<ImageView *>(widget);

			if (view)
			{
				auto [completed, total] = view->getProgress();
				progress->setProgress(completed, total);
				statusBar()->showMessage(view->activeItem());
			}
		});

		connect(tabs, &QTabWidget::tabCloseRequested, this, [this](int index) {
			auto widget = tabs->widget(index);
			auto view = qobject_cast<ImageView *>(widget);

			auto completed = 0, total = 0;

			if (view)
			{
				auto [c, t] = view->getProgress();
				completed = c;
				total = t;
			}

			tabs->removeTab(index);
			delete widget;

			if (completed < total)
			{
				// remove remaining work from total work
			}
		});

		tabs->setStyleSheet(tab_style);
		tabs->setUsesScrollButtons(false);
		tabs->setElideMode(Qt::ElideRight);
		tabs->setTabsClosable(true);

		auto status = statusBar();
		status->addPermanentWidget(progress = new ProgressWidget());
		progress->hide();

		// tabs->setTabShape(QTabWidget::TabShape::Triangular);
		readSettings();
	}

	void MainWindow::closeEvent(QCloseEvent *event)
	{
		QSettings settings;

		settings.setValue("geometry", saveGeometry());
		settings.setValue("windowState", saveState());
		settings.setValue("fitH", actions.fitH->isChecked());
		settings.setValue("fitV", actions.fitV->isChecked());
		settings.setValue("dir", lastPath);

		auto numTabs = tabs->count();
		settings.beginWriteArray("tabs", numTabs);
		for (int i = 0; i < numTabs; ++i)
		{
			settings.setArrayIndex(i);
			auto view = qobject_cast<ImageView *>(tabs->widget(i));
			if (view)
				settings.setValue("name", view->archiveName());
		}
		settings.endArray();

		settings.setValue("activeTab", tabs->currentIndex());

		QMainWindow::closeEvent(event);
	}

	void MainWindow::readSettings() noexcept
	{
		QSettings settings;

		actions.fitH->setChecked(settings.value("fitH", false).toBool());
		actions.fitV->setChecked(settings.value("fitV", false).toBool());

		auto numTabs = settings.beginReadArray("tabs");
		for (int i = 0; i < numTabs; ++i)
		{
			settings.setArrayIndex(i);
			auto filename = settings.value("name").toString();
			addTab(filename);
		}
		settings.endArray();

		auto index = settings.value("activeTab").toInt();
		tabs->setCurrentIndex(index);

		if (numTabs > 0)
			mainLayout->setCurrentIndex(1);

		restoreGeometry(settings.value("geometry").toByteArray());
		restoreState(settings.value("windowState").toByteArray());

		lastPath = settings.value("dir", QString()).toString();

		// fullscreen is part of the window state
		actions.fullscreen->setChecked(isFullScreen());
	}

	void MainWindow::addTab(const QString &filename) noexcept
	{
		LOG_INFO("Opening file: '{0}'", filename.toStdString());

		auto fileInfo = QFileInfo(filename);

		ImageView *view = new ImageView(filename, actions);

		connect(actions.open, &QAction::triggered, this, &MainWindow::fileOpen);
		connect(view, &ImageView::workStarted, this, [this](int total) {
			if (sender() == tabs->currentWidget())
				progress->setProgress(0, total);
		});
		connect(view, &ImageView::workUpdated, this, [this](int completed, int total) {
			if (sender() == tabs->currentWidget())
				progress->setProgress(completed, total);
		});
		connect(view, &ImageView::activeItemUpdated, this, [this](const QString &name) {
			if (sender() == tabs->currentWidget())
				statusBar()->showMessage(name);
		});

		int index = tabs->addTab(view, fileInfo.fileName());
		tabs->setTabToolTip(index, filename);
		tabs->setCurrentIndex(index);
		mainLayout->setCurrentIndex(1);
		progress->setProgress(0, 0);
	}

	void MainWindow::fileOpen() noexcept
	{
		auto filename = QFileDialog::getOpenFileName(this, tr("Select Image Archive"), lastPath, tr("Archive Files (*.zip *.rar *.7z *.tar.gz *.tar.bz2)"));
		if (filename.isEmpty())
			return;

		lastPath = filename;
		addTab(filename);
	}

	void MainWindow::fileClose() noexcept
	{
		auto index = tabs->currentIndex();
		if (index >= 0)
			tabs->removeTab(index);
		else
			mainLayout->setCurrentIndex(0);
	}
}
