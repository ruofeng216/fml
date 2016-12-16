#include "basewebengineview.h"
#include "WebEnginePage.h"
#include "basewebchannel.h"
#include <QUuid>
#include <QDebug>
#include "util/util.h"

BaseWebEngineView::BaseWebEngineView(QWidget *parent)
	: QWebEngineView(parent)
	, m_pWebChannel(NULL)
	, m_pWebEnginePage(new WebEnginePage(this))
	, m_isWebLoadFinished(false)
{
	this->setPage(m_pWebEnginePage);
	connect(this->page(), SIGNAL(loadFinished(bool)), this, SLOT(finish(bool)));
	connect(this->page(), SIGNAL(loadProgress(int)), this, SLOT(slotLoadProgress(int)));
	connect(this->page(), SIGNAL(sigLoadUrl(const QUrl &)), this, SLOT(hyperlinkClicked(const QUrl &)));
	setAcceptDrops(false);
	setMouseTracking(true);

	if (QSysInfo::windowsVersion() < QSysInfo::WV_WINDOWS7) {
		page()->setBackgroundColor(Qt::transparent);
	}
	else {
		page()->setBackgroundColor(QColor("#0f0f10"));
	}
	setFocusPolicy(Qt::ClickFocus);
	// ��������
	setObjectName("webview_" + QUuid::createUuid().toString());

}

BaseWebEngineView::~BaseWebEngineView()
{
}
void BaseWebEngineView::loadHtml(const QString &url)
{
	if (!url.isEmpty() && url.startsWith("qrc:/"))
	{
		page()->load(QUrl(url));
	}
	else
	{
		// ������Ƕ��ҳ
		if (url.isEmpty())
		{
			page()->load(QUrl("about:blank"));
		}
		else
		{
			page()->load(QUrl("file:///" + url));
		}
	}
}
// ���سɹ�
void BaseWebEngineView::finish(bool bsuccess)
{
	if (bsuccess)
	{
		m_isWebLoadFinished = true;

		for (int i = 0, count = m_loadFinishRunList.size(); i < count; i++) {
			runjs(m_loadFinishRunList[i].first, m_loadFinishRunList[i].second);
		}
		m_loadFinishRunList.clear();
	}
}
// ���ؽ���
void BaseWebEngineView::slotLoadProgress(int progress)
{
	qDebug() << "slotLoadProgress, url:" << this->url().toString() << " [" << progress << "]";
}
// ������Ӧ
void BaseWebEngineView::hyperlinkClicked(const QUrl &links)
{
	if (!links.isEmpty())
	{
		m_isWebLoadFinished = false;
		page()->load(links);
	}
}

// ��װrunJavaScript���������
void BaseWebEngineView::runjs(const QString &js, const JsResponseCb &cb)
{
	if (m_isWebLoadFinished) {
		if (cb) {
			page()->runJavaScript(js, cb);
		}
		else {
			page()->runJavaScript(js);
		}
	}
	else {
		bool isNextRepeat = false;
		if (!m_loadFinishRunList.isEmpty()) {
			const QPair<QString, JsResponseCb> &last = m_loadFinishRunList.last();
			isNextRepeat = last.first == js;
		}
		// ��һ���������ظ���
		if (isNextRepeat) {
			qDebug() << "runjs filter:" << js;
		}
		else {
			qDebug() << "runjs after load finished:" << js;
			m_loadFinishRunList.append(qMakePair(js, cb));
		}
	}
}


//////////////////////////////////////////////////////////////////
#include <QTimer>
//DemoWebview
DemoWebview::DemoWebview(QWidget *parent)
	: BaseWebEngineView(parent)
{
	setWebChannel();
}
DemoWebview::~DemoWebview()
{

}

void DemoWebview::setWebChannel()
{
	m_pWebChannel = new DemolWebChannel(this);
}

void DemoWebview::pushDemoData(const demoStruct &val)
{
	demoStruct t = val;
	QString str = QString("updateData('%1')").arg(json::toString(t.toJson()));
	runjs(str);
}