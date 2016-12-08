#include "webchannel.h"
#include <QWebChannel>

WebChannel::WebChannel(QObject *parent)
	: QObject(parent), m_channel(new QWebChannel(this))
{
	m_channel->registerObject("ssevent", this);
}

WebChannel::~WebChannel()
{
}

QWebChannel* WebChannel::channel()
{
	return m_channel;
}
