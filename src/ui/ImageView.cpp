#include "ImageView.hpp"

#include <QHBoxLayout>
#include <QImage>
#include <QImageReader>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPixmap>
#include <QScrollArea>
#include <QScrollBar>
#include <QSizePolicy>
#include <QThreadPool>

#include <algorithm>

#include "../Archive.hpp"
#include "../log.hpp"

constexpr auto IndexRole = Qt::UserRole + 1;
constexpr auto ImageRole = Qt::UserRole + 2;

static QListWidgetItem *findEntry(QListWidget *list, const Entry &entry)
{
	auto items = list->findItems(entry.filename, Qt::MatchFixedString);

	LOG_DEBUG("found {} items", items.size());

	// most of the time we should hit this
	if (items.size() == 1)
	{
		Q_ASSERT(items[0]->data(IndexRole).value<uint32_t>() == entry.index);
		return items[0];
	}

	auto it = std::find_if(items.begin(), items.end(), [index = entry.index](auto item)
	{
		LOG_DEBUG("looking for index {0}, checking index {1}", index, item->data(IndexRole).value<uint32_t>());
		return item->data(IndexRole).value<uint32_t>() == index;
	});

	if (it != items.end())
		return *it;

	return nullptr;
}

namespace ui
{
	ImageView::ImageView(const QString &archive, const Actions &actions, QWidget *parent) noexcept
		: fileName(archive), QWidget(parent), imageList(new QListWidget), mainImage(new QLabel), scrollArea(new QScrollArea), actions(actions)
	{
		auto mainLayout = new QHBoxLayout;

		imageList->setSortingEnabled(true);
		imageList->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
		scrollArea->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		scrollArea->setWidget(mainImage);
		mainLayout->addWidget(imageList);
		mainLayout->addWidget(scrollArea);

		setLayout(mainLayout);

		auto archiveWorker = new ReadArchiveWorker(archive, cancellationSource.get_token());

		auto resizeImage = [this] {
			showImage(imageList->currentItem(), nullptr);
		};

		connect(actions.fitH, &QAction::toggled, this, resizeImage);
		connect(actions.fitV, &QAction::toggled, this, resizeImage);

		connect(
			archiveWorker, &ReadArchiveWorker::error, this,
			[](QString msg) {
				LOG_ERROR("Archive error: {0}", msg.toStdString());
			},
			Qt::QueuedConnection);

		connect(
			archiveWorker, &ReadArchiveWorker::contents, this,
			[this](QVector<Entry> entries) {
				LOG_DEBUG("Entry names ready");

				totalFiles = entries.size();
				emit workStarted(totalFiles);

				for (auto &&entry : entries)
				{
					auto item = new QListWidgetItem(entry.filename);
					item->setData(IndexRole, entry.index);
					imageList->addItem(item);
				}
			},
			Qt::QueuedConnection);

		connect(
			archiveWorker, &ReadArchiveWorker::entryReady, this,
			[this](Entry entry, EntryData entryData) {
				LOG_DEBUG("Entry ready: #{0}: '{1}' ({2})", entry.index, entry.filename.toStdString(), entryData.type.name().toStdString());

				++totalExtracted;
				emit workUpdated(totalExtracted, totalFiles);

				auto item = findEntry(imageList, entry);
				if (!item)
				{
					LOG_ERROR("Couldn't find existing entry");
					return;
				}

				QPixmap pixmap;
				if (pixmap.loadFromData(entryData.content))
				{
					// success. Create an icon and keep the pixmap around
					// as userdata in our list so we can display it later
					item->setIcon(QIcon(pixmap));
					item->setData(ImageRole, pixmap);

					if (imageList->currentItem() == item)
						showImage(item, nullptr);
				}
				else
				{
					// fallback - try to load an icon based on the mime type
					LOG_WARN("Could not create pixmap");
					if (QIcon::hasThemeIcon(entryData.type.iconName()))
					{
						auto icon = QIcon::fromTheme(entryData.type.iconName());
						item->setIcon(icon);
					}
					else if (QIcon::hasThemeIcon(entryData.type.genericIconName()))
					{
						auto icon = QIcon::fromTheme(entryData.type.genericIconName());
						item->setIcon(icon);
					}
					else
					{
						// TODO: use icon for unknown
					}
				}
			},
			Qt::QueuedConnection);

		connect(imageList, &QListWidget::currentItemChanged, this, &ImageView::showImage);

		QThreadPool::globalInstance()->start(archiveWorker);
	}

	ImageView::~ImageView()
	{
		LOG_DEBUG("~ImageView() cancelling work");
		cancellationSource.request_stop();
	}

	QString ImageView::archiveName() const noexcept
	{
		return fileName;
	}

	QString ImageView::activeItem() const noexcept
	{
		auto current = imageList->currentItem();

		if (current)
			return current->text();

		return "";
	}

	void ImageView::showImage(QListWidgetItem *current, [[maybe_unused]] QListWidgetItem *previous) noexcept
	{
		if (!current)
		{
			LOG_DEBUG("show image called with null current item");
			mainImage->setPixmap({});
			return;
		}

		emit activeItemUpdated(current->text());

		auto pixmap = current->data(ImageRole).value<QPixmap>();

		auto viewportSize = scrollArea->maximumViewportSize();
		auto scrollSize = QSize(
			scrollArea->verticalScrollBar()->size().width(),
			scrollArea->horizontalScrollBar()->size().height()
		);

		auto availableSize = viewportSize - scrollSize;

		if (actions.fitH->isChecked() && actions.fitV->isChecked())
			pixmap = pixmap.scaled(availableSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		else if (actions.fitH->isChecked())
			pixmap = pixmap.scaledToWidth(availableSize.width(), Qt::SmoothTransformation);
		else if (actions.fitV->isChecked())
			pixmap = pixmap.scaledToHeight(availableSize.height(), Qt::SmoothTransformation);

		mainImage->setPixmap(pixmap);
		mainImage->resize(pixmap.size());
	}

	void ImageView::resizeEvent(QResizeEvent *event)
	{
		QWidget::resizeEvent(event);
		showImage(imageList->currentItem(), nullptr);
	}

}
