#include "BCGP.h"

#include "QGridLayout"
#include "QMdiArea"

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
    m_pMdiArea = new QMdiArea(centralWidget());
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
