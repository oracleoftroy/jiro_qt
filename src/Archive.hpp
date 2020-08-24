#pragma once

#include <QByteArray>
#include <QMimeType>
#include <QObject>
#include <QRunnable>
#include <QString>
#include <QVector>

#include <cstdint>

#include "stop_source.hpp"

struct Entry
{
	uint32_t index;
	QString filename;
};

struct EntryData
{
	QMimeType type;
	QByteArray content;
};

Q_DECLARE_METATYPE(Entry);
Q_DECLARE_METATYPE(EntryData);

class ReadArchiveWorker final : public QObject, public QRunnable
{
	Q_OBJECT

public:
	ReadArchiveWorker(QString file_path, stop_token token) noexcept;

private:
	void run() override;

signals:
	void error(QString msg);
	void contents(QVector<Entry> entries);
	void entryReady(Entry entry, EntryData data);

private:
	QString file_path;
	stop_token token;
};
