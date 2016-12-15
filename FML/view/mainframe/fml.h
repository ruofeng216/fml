#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_fml.h"
#include "util/datatype.h"
#include "viewsignalmanager.h"
#include <QDebug>
#include <QThread>
#include <QTimer>
#include "view/webview/basewebengineview.h"

class FML : public QMainWindow
{
    Q_OBJECT

public:
    FML(QWidget *parent = Q_NULLPTR);
	
private slots:
	void funcclick();
	void slotPushDemoData(const demoStruct &val);
private:
	void showDetialFunc(bool bShow = true);

private:
    Ui::FMLClass ui;
	bool m_showFunc;
	DemoWebview *m_pWeball;
	DemoWebview *m_pWebAdd;
};
