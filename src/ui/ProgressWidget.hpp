#pragma once

#include <QProgressBar>

namespace ui
{
	class ProgressWidget : public QProgressBar
	{
		Q_OBJECT

	public:
		ProgressWidget() noexcept;
		void setProgress(int completed, int total) noexcept;
	};
}
