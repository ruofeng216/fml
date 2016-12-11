#include "fml.h"



FML::FML(QWidget *parent)
	: QMainWindow(parent)
	, m_showFunc(false)
{
	ui.setupUi(this);
	ui.listDetailFunc->hide();
	connect(ui.funcShow, SIGNAL(clicked()), this, SLOT(funcclick()));

}

void FML::funcclick()
{
	m_showFunc = !m_showFunc;
	showDetialFunc(m_showFunc);
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