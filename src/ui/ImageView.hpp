#pragma once

#include <QString>
#include <QWidget>

#include "../stop_source.hpp"
#include "Actions.hpp"

class QLabel;
class QListWidget;
class QListWidgetItem;
class QScrollArea;
struct Entry;

namespace ui
{
	class ImageView final : public QWidget
	{
		Q_OBJECT

	public:
		explicit ImageView(const QString &archive, const Actions &actions, QWidget *parent = nullptr) noexcept;
		~ImageView() override;

		ImageView(const ImageView &) = delete;
		ImageView &operator=(const ImageView &) = delete;
		ImageView(ImageView &&) = delete;
		ImageView &operator=(ImageView &&) = delete;

		QString archiveName() const noexcept;

		auto getProgress() const noexcept
		{
			struct
			{
				int completed;
				int total;
			} result;
			result.completed = totalExtracted;
			result.total = totalFiles;

			return result;
		}

		QString activeItem() const noexcept;

	signals:
		void workStarted(int total);
		void workUpdated(int completed, int total);
		void activeItemUpdated(const QString &name);

	private:
		void showImage(QListWidgetItem *current, [[maybe_unused]] QListWidgetItem *previous) noexcept;
		void resizeEvent(QResizeEvent *event) override;

	private:
		QString fileName;
		QListWidget *imageList;
		QLabel *mainImage;
		QScrollArea *scrollArea;

		Actions actions;

		int totalExtracted = 0;
		int totalFiles = 0;

		stop_source cancellationSource;
	};
}
