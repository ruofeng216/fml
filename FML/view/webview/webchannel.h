#ifndef WEBCHANNEL_H
#define WEBCHANNEL_H

#include <QObject>
#include "webview.h"
#include "defaultweb.h"
#include "defaultmarket.h"
#include "roomfilewebview.h"

class QWebChannel;
class WebChannel : public QObject
{
	Q_OBJECT

public:
	WebChannel(QObject *parent);
	~WebChannel();

	QWebChannel* channel();

signals:
	// C++连接信号
	void sigWebChannelInit();
	void sigContextMenu(const QVariant &val);
	void sigHandle(const QString &caller, const QString &val);
	void sigMouseUp(const QVariant &val);

	// js连接信号
	void sigCppResult(const QString &caller, const QVariant &params);

public slots:
	void init() { emit sigWebChannelInit(); }
	void slotcontextmenu(const QVariant &val) { emit sigContextMenu(val); }
	void slotMouseUp(const QVariant &val) { emit sigMouseUp(val); }
	void slotHandle(const QString &caller, const QString &val) { emit sigHandle(caller, val); }

private:
	QWebChannel *m_channel;
};

// 消息
class WebViewChannel : public WebChannel
{
	Q_OBJECT;
public:
	explicit WebViewChannel(QObject *parent) : WebChannel(parent) {
		m_webview = static_cast<webview*>(parent);
		m_scrollbarPos = -1;
	}

public slots :
	void slotfileaccept(const QString &caller, const QString &msgid) { m_webview->slotfileaccept(caller, msgid); }
	void slotfileSave(const QString &caller, const QString &msgid) { m_webview->slotfileSave(caller, msgid); }
	void slotfilereject(const QString &caller, const QString &msgid) { m_webview->slotfilereject(caller, msgid); }
	void slotOfflineSend(const QString &caller, const QString &msgid) { m_webview->slotOfflineSend(caller, msgid); }
	void slotimageDbclickShow(const QString &val) { m_webview->slotimageDbclickShow(val); }
	void slotresendmsg(const QString &val) { m_webview->slotresendmsg(val); }
	void slotOpenInfoWnd(const QString &val) { m_webview->slotOpenInfoWnd(val); }
	void slotOpenFilepath(const QString &val) { m_webview->slotOpenFilepath(val); }
	void slotOpenFile(const QString &val) { m_webview->slotOpenFile(val); }
	void slotOpenChatto(const QString &val) { m_webview->slotOpenChatto(val); }
	void slotsearchmore() { m_webview->slotsearchmore(); }
	void slotDownHistory() { m_webview->slotDownHistory(); }
	void slotUpHistory() { m_webview->slotUpHistory(); }
	void slotopenRecordtoSearchMore() { m_webview->slotopenRecordtoSearchMore(); }
	void slothistoryPage(const QString &val) { m_webview->slothistoryPage(val); }
	void slotCreditoright(const QString &val) { m_webview->slotCreditoright(val); }
	void slotSubscribe(const QString &caller, const QString &val) { m_webview->slotSubscribe(caller, val); }
	void slotGrabPixmap(const QString &val) { m_webview->slotGrabPixmap(val); }
	void slotRoomcard(const QString &caller, const QString &val) { m_webview->slotRoomcard(caller, val); }
	void slotBondOpt(const QString &val) { m_webview->slotBondOpt(val); }
	void slotopenurl(const QString &val) { m_webview->slotopenurl(val); }
	void slotUpdateBigTxtMsgShow(const QString &val) { m_webview->slotUpdateBigTxtMsgShow(val); }
	void slotShowGroupMassReceivers(const QString &val) { m_webview->slotShowGroupMassReceivers(val); }
	void slotStartDrag(const QString &type, const QString &val) { m_webview->slotStartDrag(type, val); }
	void slotScrollbarPos(int pos) { m_scrollbarPos = pos; }
	int slotScrollbarPos() { return m_scrollbarPos; }

private:
	friend class webview;
	webview *m_webview;
	int m_scrollbarPos;
};

// 机构号新闻
class DefaultWebChannel : public WebChannel
{
	Q_OBJECT
public:
	explicit DefaultWebChannel(QObject *parent) : WebChannel(parent) {
	}
};

// 市场资讯
class DefaultMarketChannel : public WebChannel {
	Q_OBJECT;
public:
	explicit DefaultMarketChannel(QObject *parent) : WebChannel(parent) {
		m_webview = static_cast<DefaultMarket*>(parent);
	}

public slots :
	void slotMoreNewsOpen() { m_webview->slotMoreNewsOpen(); }
	void slotStatisticPoint(const QString &val) { m_webview->slotStatisticPoint(val); }

private:
	friend DefaultMarket;
	DefaultMarket *m_webview;
};

#endif // WEBCHANNEL_H
