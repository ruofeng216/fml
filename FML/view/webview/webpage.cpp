#include "webpage.h"
#include "qmlogger.h"
#include <QAuthenticator>
#include "qutil.h"
#include "controller_manager.h"

WebPage::WebPage(QObject *parent) 
	: QWebEnginePage(parent)
{
	connect(this, SIGNAL(authenticationRequired(const QUrl &, QAuthenticator*)),
		SLOT(authenticationRequired(const QUrl &, QAuthenticator*)));
	connect(this, SIGNAL(proxyAuthenticationRequired(const QUrl &, QAuthenticator *, const QString &)),
		SLOT(proxyAuthenticationRequired(const QUrl &, QAuthenticator *, const QString &)));
}

void WebPage::updateAuth(QAuthenticator *auth)
{
	if (auth) {
		CProxyInfo proxyInfo;
		ControllerManager::instance()->getGlobalInstance()->GetProxy(proxyInfo);
		if (proxyInfo.GetPoxyType() != Proxy_Type_None) {
			auth->setUser(qutil::toQString(proxyInfo.GetUser()));
			auth->setPassword(qutil::toQString(proxyInfo.GetPass()));
		}
	}
}

bool WebPage::acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame)
{
	if (isMainFrame) {
		if (NavigationTypeLinkClicked == type) {
			emit sigLoadUrl(url);
			return false;
		}
	}
	return true;
}

void WebPage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &message, int lineNumber, const QString &sourceID)
{
	QString levelStr = (level == 0 ? "INFO" : (level == 1 ? "WARN" : "ERROR"));
	QString htmlName = sourceID;
	int findPos = sourceID.lastIndexOf("/");
	if (findPos >= 0) {
		htmlName = sourceID.mid(findPos + 1);
	}
	QString webLog = QString("[WEBLOG level:%1,line:%2,name:%3] %4").arg(levelStr).arg(lineNumber).arg(htmlName).arg(message);
	qDebug() << webLog;
	LOG_INFO << webLog.toLocal8Bit().data();
}

QWebEnginePage *WebPage::createWindow(WebWindowType type)
{
	emit sigCreateWindow(type);
	return nullptr;
}

void WebPage::authenticationRequired(const QUrl &requestUrl, QAuthenticator *auth)
{
	LOG_INFO << "authenticationRequired, url:" << requestUrl.toString().data();
	updateAuth(auth);
}

void WebPage::proxyAuthenticationRequired(const QUrl &requestUrl, QAuthenticator *auth, const QString &proxyHost)
{
	LOG_INFO << "proxyAuthenticationRequired, url:" << requestUrl.toString().data() << ", proxyHost:" << proxyHost.data();
	updateAuth(auth);
}
