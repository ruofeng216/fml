#pragma once

#include <QtWebEngineWidgets/QWebEngineView>
#include <QHash>
#include <functional>
#include <QtWebEngineCore/QWebEngineCallback>
#include "util/datatype.h"



class BaseWebChannel;
class WebEnginePage;
class BaseWebEngineView : public QWebEngineView
{
	Q_OBJECT

public:
	typedef std::function<void(const QVariant &)> JsResponseCb;
	explicit BaseWebEngineView(QWidget *parent);
	~BaseWebEngineView();

	// 包装runJavaScript，方便管理
	void runjs(const QString &js, const JsResponseCb &cb = nullptr);

private slots:
	// 加载成功
	void finish(bool bsuccess);
	// 加载进度
	void slotLoadProgress(int progress);
	// 链接响应
	void hyperlinkClicked(const QUrl &);

protected:
	virtual void setWebChannel() = 0;

protected:
	BaseWebChannel *m_pWebChannel;
	WebEnginePage *m_pWebEnginePage;
	bool m_isWebLoadFinished;
	QList<QPair<QString, JsResponseCb>> m_loadFinishRunList;
};

class DemoWebview : public BaseWebEngineView
{
	Q_OBJECT

public:
	explicit DemoWebview(QWidget *parent);
	~DemoWebview();

	void pushDemoData(const demoStruct &val);

private:
	void setWebChannel();
};