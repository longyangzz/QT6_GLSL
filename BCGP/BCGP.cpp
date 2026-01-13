#include "BCGP.h"

#include "QGridLayout"
#include "MdiArea.h"

#include "GLSLViewer/GLSLViewer.h"

static BCGP* s_instance = nullptr;

//! 实例化对象
BCGP* BCGP::Instance()
{
    //实例化MyClass1 
    if (!s_instance)
    {
        s_instance = new BCGP();
    }

    return s_instance;
}
//! 删除实例化的对象
void BCGP::UnInitialize()
{
    if (s_instance)
        delete s_instance;

    s_instance = 0;
}


BCGP::BCGP(QWidget *parent)
    : DCGui::AuxMainWindow(parent, Qt::WindowFlags())
    , m_pMdiArea(nullptr)
{
    bool status = ConfigInit(this);

    if (status)
    {
        Init();
    }

    ConfigFinish(this);
}

BCGP::~BCGP()
{}

void BCGP::Init()
{
    //初始化多窗口管理
    QGridLayout* gridLayout = new QGridLayout(centralWidget());
    m_pMdiArea = new MdiArea(centralWidget());
    m_pMdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_pMdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_pMdiArea->setFocus();
    gridLayout->addWidget(m_pMdiArea, 0, 0, 1, 1);

    //设置底板背景图片
    m_pMdiArea->setBackground(Qt::NoBrush);
    QBrush bgBrush(QImage(":/img/Resources/gpbg.png"));
    m_pMdiArea->setBackground(bgBrush);

    //! 激活多窗口的拖放操作
    m_pMdiArea->setAcceptDrops(true);

    connect(m_pMdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(ChangedCurrentViewer(QMdiSubWindow*)));

    //状态栏
    statusBar()->showMessage(QString::fromLocal8Bit("ok"));
}

//! 加载文件
int BCGP::LoadFile(const QString& fileName, GLSLViewer* viewer)
{
	
	return 0;
}

//! 在当前激活窗口中，添加数据文件
int BCGP::AddFile(GLSLViewer* viewer, const QString& fileName)
{
	
	return 1;
}

//! 打开工程+
void BCGP::OpenProject(QString proName)
{
	
}

//! 返回当前激活的窗口
QWidget* BCGP::ActiveMdiChild()
{
    if (QMdiSubWindow* activeSubWindow = m_pMdiArea->currentSubWindow())
    {
        return qobject_cast<QWidget*>(activeSubWindow->widget());
    }
    else
    {
        return nullptr;
    }
}