#pragma once

#include <QObject>
#include <QtWebEngineCore/QWebEngineCallback>

class QWebChannel;
class BaseWebChannel : public QObject
{
	Q_OBJECT

public:
	explicit BaseWebChannel(QObject *parent);
	virtual ~BaseWebChannel();

	const QWebChannel *channel() const;

public slots:
	virtual const QVariant slotHandle(const QVariant &val)=0;
private:
	QWebChannel *m_channel;
};


class DemolWebChannel : public BaseWebChannel
{
	Q_OBJECT
public:
	explicit DemolWebChannel(QObject *parent);
	~DemolWebChannel();
	int getID() { return ++m_id; }
public slots:
	const QVariant slotHandle(const QVariant &val);
private:
	int m_id;
};
