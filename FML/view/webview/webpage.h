#pragma once

#include <QWebEnginePage>

class WebPage : public QWebEnginePage
{
	Q_OBJECT
public:
	explicit WebPage(QObject *parent = 0);
	static void updateAuth(QAuthenticator *auth);

signals:
	void sigLoadUrl(const QUrl &url);
	void sigCreateWindow(WebWindowType type);

protected:
	bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame);
	void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &message, int lineNumber, const QString &sourceID);
	QWebEnginePage *createWindow(WebWindowType type);

private slots:
	void authenticationRequired(const QUrl &, QAuthenticator*);
	void proxyAuthenticationRequired(const QUrl &requestUrl, QAuthenticator *authenticator, const QString &proxyHost);
};