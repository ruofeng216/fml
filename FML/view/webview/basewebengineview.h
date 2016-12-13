#pragma once

#include <QWebEngineView>
#include <QHash>
#include <functional>

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
	const void runjs(const QString &js, const JsResponseCb &cb = nullptr);

private slots:
	// 加载成功
	void finish(bool bsuccess);
	// 加载进度
	void slotLoadProgress(int progress);
	// 链接响应
	void hyperlinkClicked(const QUrl &);

private:
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

private:
	void setWebChannel();
};