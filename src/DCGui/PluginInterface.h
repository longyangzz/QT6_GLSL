#ifndef DCCORE_PLUGININTERFACE_H
#define DCCORE_PLUGININTERFACE_H

//Qt
#include "QObject"
#include "QVector"
#include "QList"
#include "QMap"

class QMenu;
class QToolBar;
class QWidget;
class QTreeWidgetItem;
class QDockWidget;


class PluginInterface : public QObject
{
public:
	//模块菜单
	virtual QList<QMenu*> Menus() const = 0;
	//模块工具条
	virtual QList<QToolBar*> ToolBars() const = 0;
	//停靠窗体
	virtual QMap<QDockWidget*, Qt::DockWidgetArea> DockWidgets() const = 0;

	//选项根节点
	virtual QTreeWidgetItem* OptionRoot() const = 0;
	//选项窗体
	virtual QMap<QTreeWidgetItem*, QWidget*> OptionWidgets() const = 0;
	//模块指定的在菜单栏显示名字
	virtual QString ModuleName() const = 0;

	//模块指定工具栏停靠位置 1 2 4 8 （左右上下）
	virtual int ToolBarPosArea()
	{
		return 4;
	}

	/*******************以下信号和槽根据项目需求自行添加*************************/
//	//slots:
//		//改变待处理数据
//		void ChangedHandlingEntities(const QVector<DcGp::DcGpEntity*>& entities);
//
//signals:
//	//发送处理后的数据
//	void EntitiesGenerated(const QVector<DcGp::DcGpEntity*>& entities);
//	//执行命令
//	void ExecuteCommand(const QStringList& commands);
};

#define PluginInterface_iid "com.bjdclw.DC-Platform.PluginInterface/6.0"
Q_DECLARE_INTERFACE(PluginInterface, PluginInterface_iid)

#endif