#include "fml.h"
#include "util/util.h"
#include "controller/qcontrollermanager.h"
#include "view/view_controller.h"


FML::FML(QWidget *parent)
	: QMainWindow(parent)
	, m_showFunc(false)
	, m_pWeball(NULL)
	, m_pWebAdd(NULL)
{
	ui.setupUi(this);
	ui.listDetailFunc->hide();
	connect(ui.funcShow, SIGNAL(clicked()), this, SLOT(funcclick()));

	m_pWeball = new DemoWebview(this);
	m_pWebAdd = new DemoWebview1(this);

	ui.gridLayout_3->addWidget(m_pWeball);
	ui.gridLayout_4->addWidget(m_pWebAdd);

	connect(ViewController::instance(), &ViewController::pushDemoData, this, &FML::slotPushDemoData);

	m_pWeball->loadHtml(qutil::websrc("web/demo/1.html"));
	m_pWebAdd->loadHtml(qutil::websrc("web/demo/2.html"));
}

void FML::funcclick()
{
	m_showFunc = !m_showFunc;
	showDetialFunc(m_showFunc);
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