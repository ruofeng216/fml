#include "fml.h"
#include "util/util.h"
#include "controller/qcontrollermanager.h"


FML::FML(QWidget *parent)
	: QMainWindow(parent)
	, m_showFunc(false)
	, m_pWeball(NULL)
	, m_pWebAdd(NULL)
{
	ui.setupUi(this);
	ui.listDetailFunc->hide();
	connect(ui.funcShow, SIGNAL(clicked()), this, SLOT(funcclick()));

	m_pWeball = new DemoWebview(ui.showwidget);
	//m_pWebAdd = new DemoWebview(ui.showwidget);

	ui.gridLayout->addWidget(m_pWeball);
	//gridLayout->addWidget(m_pWebAdd, 0, 0, 1, 1);

	m_pWeball->loadHtml(qutil::websrc("web/demo/1.html"));
}

void FML::funcclick()
{
	m_showFunc = !m_showFunc;
	showDetialFunc(m_showFunc);
	QControllerManager::instance()->init();
}

void FML::slotPushDemoData(const demoStruct &val)
{
	if (m_pWeball)
	{
		m_pWeball->pushDemoData(val);
	}
}

void FML::showDetialFunc(bool bShow)
{
	if (bShow)
	{
		// 获取大功能选中项对应详细功能项刷新展示

		ui.listDetailFunc->show();
	}
	else
	{
		ui.listDetailFunc->hide();
	}
}