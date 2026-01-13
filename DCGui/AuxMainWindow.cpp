//##########################################################################
//#                                                                        #
//#                            DCLW                                        #
//#                                                                        #
//#  This library is free software; you can redistribute it and/or modify  #
//#  it under the terms of the GNU Lesser General Public License(LGPL) as  #
//#  published by the Free Software Foundation; version 2 of the License.  #
//#                                                                        #
//#  This library is distributed in the hope that it will be useful,       #
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of        #
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
//#  GNU Lesser General Public License for more details.                   #
//#                                                                        #
//#          COPYRIGHT: DCLW             																	 #
//#                                                                        #
//##########################################################################

/*==========================================================================
*【功能概述】
*
*作者：dclw         时间：$time$
*文件名：$safeitemname$
*版本：V2.1.0
*
*修改者：          时间：
*修改说明：
*===========================================================================
*/

#include "AuxMainWindow.h"

//QT
#include "QApplication"
#include "QDir"
#include "QList"
#include "QSettings"
#include "QPluginLoader"
#include "QDebug"
#include "QMenuBar"
#include "QMessageBox"
#include "QDesktopServices"
#include "QUrl"
#include "QFile"
#include "QFileInfo"
#include "QDomDocument"

using namespace DCGui;

AuxMainWindow::AuxMainWindow(QWidget *parent, Qt::WindowFlags flags)
	: IWindow(parent, flags)
{
	
}

AuxMainWindow::~AuxMainWindow()
{
	//移除所有的子对象，并delete
	while(!m_Module.empty())
	{
		PluginInterface* child = m_Module.back();
		m_Module.pop_back();

		delete child;
		child = nullptr;
	}
}

//加载模块
void AuxMainWindow::LoadModules()
{
	//模块路径
	QDir moduleDir = QApplication::applicationDirPath() + "/Plugins";
	QString aa = QApplication::applicationDirPath();
	//模块列表
	QStringList modules = moduleDir.entryList(QStringList() << "*.mdl" << "*.dll",

		QDir::NoFilter, QDir::Time | QDir::Reversed);

	for (auto iter = modules.constBegin();
		iter != modules.constEnd(); ++iter)
	{
		
		//模块加载器
		QPluginLoader loader(moduleDir.absoluteFilePath(*iter));

		//如果模块可识别，则加载
		PluginInterface* pModule = qobject_cast<PluginInterface*>(loader.instance());

		qDebug() << loader.errorString() << Qt::endl;

		if (pModule)
		{
			//连接模块的信号
			ConnectModule(pModule);

			//加载模块菜单和工具栏
			LoadMenusAndToolBars(pModule->ModuleName(), pModule->Menus(), pModule->ToolBars(), pModule->ToolBarPosArea());
			//加载停靠窗体
			LoadDockWidgets(pModule->DockWidgets());

			m_Module.push_back(pModule);
		}
	}
}

//连接模块的信号
void AuxMainWindow::ConnectModule(PluginInterface* pModule)
{
	//获取元对象
	const QMetaObject* pMeta = pModule->metaObject();

	//判断执行命令的信号是否存在，若存在，则连接
	if (pMeta->indexOfSignal("ExecuteCommand(QStringList)") != -1)
	{
		connect(pModule, SIGNAL(ExecuteCommand(const QStringList&)),
			this, SLOT(ExecuteCommand(const QStringList&)));
	}

	//判断生成新对象的信号是否存在，若存在，则连接
	if (pMeta->indexOfSignal("EntitiesGenerated(QVector<DcGp::DcGpEntity*>,bool)") != -1)
	{
		connect(pModule, SIGNAL(EntitiesGenerated(const QVector<DcGp::DcGpEntity*>&, bool)),
			this, SLOT(GeneratedEntities(const QVector<DcGp::DcGpEntity*>&, bool)));
	}

	//判断生成新对象的信号是否存在，若存在，则连接
	if (pMeta->indexOfSignal("EntitiesGenerated(QVector<DcGp::DcGpEntity*>)") != -1)
	{
		connect(pModule, SIGNAL(EntitiesGenerated(const QVector<DcGp::DcGpEntity*>&)),
			this, SLOT(GeneratedEntities(const QVector<DcGp::DcGpEntity*>&)));
	}

	//判断当前对象变化的槽是否存在，若存在，则连接
	if (pMeta->indexOfSlot("ChangedHandlingEntities(QVector<DcGp::DcGpEntity*>)") != -1)
	{
		connect(this, SIGNAL(HandlingEntitiesChanged(const QVector<DcGp::DcGpEntity*>&)),
			pModule, SLOT(ChangedHandlingEntities(const QVector<DcGp::DcGpEntity*>&)));
	}

	//判断当前view变化，若存在，则连接
	if (pMeta->indexOfSlot("ChangedCurrentViewer(DCView::View*)") != -1)
	{
		connect(this, SIGNAL(CurrentViewerChanged(DCView::View*)),
			pModule, SLOT(ChangedCurrentViewer(DCView::View*)));
	}
}

//加载菜单栏和工具栏
void AuxMainWindow::LoadMenusAndToolBars(QString moduleName,QList<QMenu*> menus,
	QList<QToolBar*> toolBars, int toolposArea)
{

	//获取root菜单个数
	int rootNum = GetRootMenuNum();
	if (rootNum == 0)
	{
		return;
	}
	//获取倒数第二个菜单
	QAction* actionBefore = GetAction(rootNum - 1);

	//获取菜单栏
	QMenuBar* pMenuBar = menuBar();

	//！ 判断当前模块菜单的个数
	int menuNums = menus.size();

	//! 根据moduleName是否为空决定是横排放置菜单，还是竖排放置
	//! 解析全局配置文件中的setting.xml中的itemstyle配置值
	QString itemstyle = getGlobleSettingFieldValue("menu", "itemstyle", "expand");

	if (!moduleName.isEmpty() && itemstyle != "expand")   //! 创建新的模块菜单
	{
		//! 创建模块菜单名
		QMenu* moduleMenu = new QMenu;
		moduleMenu->setObjectName(moduleName);
		moduleMenu->setTitle(moduleName);
		pMenuBar->insertMenu(actionBefore, moduleMenu);


		//菜单
		for (auto iterMenu = menus.constBegin();
			iterMenu != menus.constEnd(); ++iterMenu)
		{
			moduleMenu->addMenu(*iterMenu);
		}
	}
	else
	{
		//菜单
		for (auto iterMenu = menus.constBegin();
			iterMenu != menus.constEnd(); ++iterMenu)
		{
			pMenuBar->insertMenu(actionBefore, *iterMenu);
		}
	}
	

	//工具栏
	for (auto iterToolBar = toolBars.constBegin();
		iterToolBar != toolBars.constEnd(); ++iterToolBar)
	{
		addToolBar((Qt::ToolBarArea)toolposArea,*iterToolBar);
	}
}

//加载停靠窗体
void AuxMainWindow::LoadDockWidgets(QMap<QDockWidget*, Qt::DockWidgetArea> docks)
{
	if (docks.isEmpty())
	{
		return;
	}

	for (auto iter = docks.constBegin();
		iter != docks.constEnd(); ++iter)
	{
		//添加停靠窗体
		addDockWidget(iter.value(), iter.key());
	}
}

//注册命令
//void AuxMainWindow::RegisterCommands(QMap<QString, DCCore::CommandGenerator> generators)
//{
//	//命令管理器
//	CommandManager* pManager = CommandManager::Instance();
//	pManager->RegisterCommands(generators);
//}

void AuxMainWindow::About()
{
	QMessageBox::about(this,            // parent
		tr("About"),     // title
		tr("My Application v1.0\n© 2026")); // text
}

void AuxMainWindow::Help()
{
	//应用程序名字
	QString eName = QApplication::applicationName();

	//支持的后缀名
	QStringList fixName;
	fixName << ".pdf" << ".doc" << ".docx" << ".chm";

	//要打开的帮助文件全名
	QString strHelpFile;
	//! 遍历查找打开
	for (int i = 0; i != fixName.size(); ++i)
	{
		strHelpFile = QApplication::applicationDirPath() + "/" + eName + fixName[i];
		QFileInfo file(strHelpFile);
		if (file.exists())
		{
			//打开帮助文件
			QDesktopServices desktop;
			desktop.openUrl(QUrl(strHelpFile));
			break;
		}
	}
}