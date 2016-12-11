#include "login.h"

login::login(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	ui.leUsername->setPlaceholderText(tr("input name"));
	ui.lePassword->setPlaceholderText(tr("input password"));
}

login::~login()
{
}
