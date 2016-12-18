#include "basewebchannel.h"
#include <QWebChannel>
#include "webviewsever.h"
#include "websockettransport.h"
#include <QDebug>

BaseWebChannel::BaseWebChannel(QObject *parent)
	: QObject(parent)
{
	m_channel = new QWebChannel(this);
	m_channel->registerObject("CallCpp", this);
	QObject::connect(WEBVIEWSEVER->getWrapper(), &WebSocketClientWrapper::clientConnected,
		m_channel, &QWebChannel::connectTo);
}

BaseWebChannel::~BaseWebChannel()
{
}

const QWebChannel *BaseWebChannel::channel() const
{
	return m_channel;
}

///////////////////////////////////////////////////////////
// DemolWebChannel
DemolWebChannel::DemolWebChannel(QObject *parent)
	: BaseWebChannel(parent)
{
}
DemolWebChannel::~DemolWebChannel()
{
}

const QVariant DemolWebChannel::slotHandle(const QVariant &val)
{
	qDebug() << "C++ get js push : " << val;
	QString s = val.toString();
	return QString("c++ return js : %1 ").arg(s) ;
}
