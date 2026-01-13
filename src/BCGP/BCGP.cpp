#include "BCGP.h"

#include "QGridLayout"
#include "MdiArea.h"
#include "QSettings"
#include "QFileDialog"
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

GLSLViewer* BCGP::CurrentDCViewer()
{
    GLSLViewer* pViewer = static_cast<GLSLViewer*>(ActiveMdiChild());
    return pViewer;
}

//! 改变当前窗体
void BCGP::ChangedCurrentViewer(QMdiSubWindow* subWindow)
{
    GLSLViewer* pViewer = CurrentDCViewer();
    //发送消息信号通知窗口改变了
}

//! 加载文件
int BCGP::LoadFile(const QString& fileName, GLSLViewer* viewer)
{
    //!2 创建一个view，视窗与view共享场景根节点
    GLSLViewer* pNewViewer = new GLSLViewer(this);
    
    //判断是否存在窗口
    QList<QMdiSubWindow*> subWindowList = m_pMdiArea->subWindowList();
    GLSLViewer* otherWin = 0;
    if (!subWindowList.isEmpty())
    {
        otherWin = static_cast<GLSLViewer*>(subWindowList[0]->widget());
    }

    QMdiSubWindow* subWindow = m_pMdiArea->addSubWindow(pNewViewer);
    subWindow->setAttribute(Qt::WA_DeleteOnClose);
    subWindow->setMinimumSize(300, 200);
    subWindow->setWindowTitle("GLSL Viewer");
    
    //最大化显示，初始化opengl环境后才能加载数据
    subWindow->showMaximized();

    pNewViewer->loadPointCloud(fileName);
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

//! 导入点云数据到新的站点
void BCGP::ImportData()
{
    //记录文件路径
    QSettings settings;
    settings.beginGroup("ImporData");
    QString currentPath = settings.value("ImporDataPath", QApplication::applicationDirPath()).toString();

    //! 选取文件名
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Open file"), currentPath);

    if (fileNames.isEmpty())
    {
        return;
    }

    //! 读取数据，并创建窗口，添加到管理模块中
    for (int i = 0; i != fileNames.size(); ++i)
    {
        LoadFile(fileNames[i], nullptr);
    }

    //更新当前保存的文件路径和文件过滤类型值
    currentPath = QFileInfo(fileNames[0]).absolutePath();
    //将更新的值保存到settings中
    settings.setValue("ImporDataPath", currentPath);
    settings.endGroup();
}