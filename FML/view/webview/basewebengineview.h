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

	// ��װrunJavaScript���������
	const void runjs(const QString &js, const JsResponseCb &cb = nullptr);

private slots:
	// ���سɹ�
	void finish(bool bsuccess);
	// ���ؽ���
	void slotLoadProgress(int progress);
	// ������Ӧ
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