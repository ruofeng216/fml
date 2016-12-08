#ifndef WEBVIEWWIDGET_H
#define WEBVIEWWIDGET_H

#include "webview.h"

class QHBoxLayout;
class WebViewWidget : public QWidget
{
	Q_OBJECT

public:
	explicit WebViewWidget(QWidget *parent = 0);
	void init(const std::function<webview*()> &createFunc, const std::function<void(webview *view)> &connectFunc = nullptr);
	void addTask(const std::function<void(webview *view)> &task);
	webview* view();

	void load();
	void unload();

private slots:
	void slotInit();
	void slotRenderProcessTerminated(QWebEnginePage::RenderProcessTerminationStatus, int);

private:
	webview *m_webview;
	std::function<webview*()> m_createFunc;
	std::function<void(webview *view)> m_connectFunc;
	QList<std::function<void(webview *view)>> m_tasks;
	QHBoxLayout *m_hLayout;
};
#endif // WEBVIEWWIDGET_H
