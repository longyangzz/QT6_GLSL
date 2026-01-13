#include "BCGP.h"
#include <QtWidgets/QApplication>
#include "QFileInfo"
int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	int aa = sizeof(long);

	//! 判断许可，使用数据库
	BCGP* mw = BCGP::Instance();
	QString license = mw->GetLicenseState();
	mw->SetLicenseText(license);

	//! 判断DCLCMG许可
#ifdef USE_LISCENSE
	//判断许可状态
	if (!IsValidLicense())
	{
		//卸载主mw
		mw->UnInitialize();
		return EXIT_FAILURE;
	}
#endif

	mw->showMaximized();

	//! 支持工程文件打开，解析命令行参数
	if (argc >= 2)
	{
		QString proName = argv[1];
		QFileInfo info(proName);
		if (info.suffix().toUpper() == "DCPRO")
		{
			//! 打开工程文件
			mw->OpenProject(proName);
		}
		else
		{
			//! 打开支持的文件
			mw->LoadFile(proName, nullptr);
		}
	}

	int result = 0;
	try
	{
		result = a.exec();
	}
	catch (...)
	{
		result = a.exec();
	}

	//卸载主mw
	mw->UnInitialize();

	return result;
}
