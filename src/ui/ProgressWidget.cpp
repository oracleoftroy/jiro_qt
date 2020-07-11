#include "ProgressWidget.hpp"

#include <QSizePolicy>

namespace ui
{
	ProgressWidget::ProgressWidget() noexcept
	{
		setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
		setMaximumWidth(75);
		setTextVisible(false);
	}

	void ProgressWidget::setProgress(int completed, int total) noexcept
	{
		if (total != 0 && completed == total)
			hide();
		else
		{
			setMinimum(0);
			setMaximum(total);
			setValue(completed);
			show();
		}
	}
}
