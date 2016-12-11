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
		// ��ȡ����ѡ�����Ӧ��ϸ������ˢ��չʾ

		ui.listDetailFunc->show();
	}
	else
	{
		ui.listDetailFunc->hide();
	}
}