#include "webviewwidget.h"
#include <QHBoxLayout>
#include <QTimer>
#include <QApplication>

WebViewWidget::WebViewWidget(QWidget *parent)
	: QWidget(parent), m_webview(nullptr)
{
	m_hLayout = new QHBoxLayout(this);
	m_hLayout->setContentsMargins(0, 0, 0, 0);
	m_hLayout->setSpacing(0);
	connect(qApp, &QApplication::aboutToQuit, this, &WebViewWidget::unload);
}

void WebViewWidget::init(const std::function<webview*()> &createFunc, const std::function<void(webview *view)> &connectFunc)
{
	m_createFunc = createFunc;
	m_connectFunc = connectFunc;
}

void WebViewWidget::addTask(const std::function<void(webview*)> &task)
{
	if (m_webview && m_webview->isInit()) {
		task(m_webview);
	} else {
		m_tasks.append(task);
	}
}

webview* WebViewWidget::view()
{
	return m_webview;
}

void WebViewWidget::load()
{
	if (!m_webview && m_createFunc) {
		m_webview = m_createFunc();
		connect(m_webview, SIGNAL(s_init()), this, SLOT(slotInit()));
		connect(m_webview, SIGNAL(renderProcessTerminated(QWebEnginePage::RenderProcessTerminationStatus, int)),
			this, SLOT(slotRenderProcessTerminated(QWebEnginePage::RenderProcessTerminationStatus, int)));
		if (m_connectFunc) {
			m_connectFunc(m_webview);
		}
		m_hLayout->addWidget(m_webview);
		m_webview->loadUrl("");
		m_webview->show();
	}
}

void WebViewWidget::unload()
{
	if (m_webview) {
		m_hLayout->removeWidget(m_webview);
		m_webview->deleteLater();
		m_webview = nullptr;
	}
}

void WebViewWidget::slotInit()
{
	if (m_webview) {
		for (int i = 0, count = m_tasks.size(); i < count; i++) {
			m_tasks[i](m_webview);
		}
		m_tasks.clear();
	}
}

void WebViewWidget::slotRenderProcessTerminated(QWebEnginePage::RenderProcessTerminationStatus status, int code)
{
	LOG_ERROR << "renderProcessTerminated, status:" << status << ", exitCode:" << code;
	unload();
	load();
}