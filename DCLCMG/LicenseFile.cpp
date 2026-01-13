#include "LicenseFile.h"

using namespace DcLCMG;

#include <Windows.h>

//Qt
#include "QString"
#include "QDir"
#include "QFile"
#include "QDataStream"
#include "QDate"

#include "QCoreApplication"
#include "QCryptographicHash"


#include "HardwareInfo.h"

void EncryptLicenseFileName(QString& strName)
{
	for (auto it = strName.begin(); it != strName.end(); ++it)
	{
		char ch = it->unicode();
		ch += (it - strName.begin());
		*it = QChar(ch);
	}
}

LicenseFile LicenseFile::s_instance;

LicenseFile::LicenseFile()
	: m_daysOfTrial(15)
	, m_existType(eFileNotExist)
	, m_licenseState(eTrial)
	, m_organizationName("DCLW")
	, m_applicationName("Test")
	, m_applicationVersion("V1.0.0")
{

}

//许可状态
int LicenseFile::StateOfLicense()
{
	return s_instance.GetLicenseState();
}

//设置公司名称
void LicenseFile::SetOrganizationName(const QString& strOrganizationName)
{
	s_instance.m_organizationName = strOrganizationName;
}

//设置应用程序名称
void LicenseFile::SetApplicationName(const QString& strApplicationName)
{
	s_instance.m_applicationName = strApplicationName;
}

//设置应用程序版本
void LicenseFile::SetApplicationVersion(const QString& strApplicationVersion)
{
	s_instance.m_applicationVersion = strApplicationVersion;
}

//设置试用天数
void LicenseFile::SetTrialDays(unsigned days)
{
	s_instance.m_daysOfTrial = days;
}

//许可文件名
QString LicenseFile::LicenseFileName()
{
	//如果许可文件名为空，则生成许可文件名
	if (m_licenseFileName.isEmpty())
	{
		//获取公司名称
		QString organizationName = m_organizationName;

		//去除公司名称中的空格
		organizationName.remove(' ');

		//对公司名进行加密
		EncryptLicenseFileName(organizationName);

		//生成许可文件名
		const QString strLicense = organizationName + ".dll";

#ifdef Q_OS_WIN
		//获取Windows路径
		wchar_t* pPathWindows = new wchar_t[10000];
		GetWindowsDirectoryW(pPathWindows, 10000);

		//生成许可文件的绝对路径名
		const QString strWindowsPath = QDir::toNativeSeparators(QCoreApplication::applicationDirPath());
		const QString strLicenseFile = strWindowsPath + "\\" + strLicense;
#endif

		//保存许可文件的绝对路径名
		m_licenseFileName = strLicenseFile;
	}
	
	//返回许可文件的绝对路径名
	return m_licenseFileName;
}

//许可是否存在
bool LicenseFile::IsExist()
{
	//第一步：先判断许可文件是否存在
	const QString strLicenseFile = LicenseFileName();

	//如果许可文件不存在，则无许可
	if (!QFile::exists(strLicenseFile))
	{
		m_existType = eFileNotExist;
		return false;
	}

	//第二步：判断许可文件中是否存在许可
	if (!ExistLicenseInFile())
	{
		m_existType = eLicenseNotExist;

		return false;
	}

	return true;
}

//创建许可文件
void LicenseFile::Init()
{
	//获取许可文件名称
	const QString strLicenseFile = LicenseFileName();
	//
	QFile file(strLicenseFile);
	QDataStream out;

	//若许可文件不存在，则创建新的许可文件
	if (m_existType == eFileNotExist)
	{
		file.open(QIODevice::WriteOnly);
		out.setDevice(&file);

		//魔术数字
		out << (qint64)0x19911211;

		//qt版本信息
		out << (qint64)QDataStream::Qt_4_8;

		out.setVersion(QDataStream::Qt_4_8);
	}
	//如果许可文件存在，但对应版本的应用程序许可信息不存在
	//则在许可文件中追加许可信息
	else if (m_existType == eLicenseNotExist)
	{
		file.open(QIODevice::Append);
		out.setDevice(&file);
	}

	//输出应用程序名称和版本
	out << m_applicationName;
	out << m_applicationVersion;

	//第一次启动软件的日期
	QDate srcDate = QDate(1900, 1, 1);
	out << (qint64)(srcDate.daysTo(QDate::currentDate()));
	
	//软件过期时间（软件注册期为一年,只有当软件状态为激活时，此项有效）
	out << (qint64)(srcDate.daysTo(QDate::currentDate()) + m_daysOfTrial);

	//软件的许可时间
	out << (qint64)m_daysOfTrial;

	//软件许可状态
	out << (qint64)eTrial;
	m_licenseState = eTrial;

	//关闭文件
	file.close();
}

//获取许可状态
LicenseState LicenseFile::GetLicenseState()
{
	if (!IsExist())
	{
		Init();
	}

	return m_licenseState;
}

//
bool LicenseFile::ExistLicenseInFile()
{
	//获取许可文件名
	const QString strLicenseFile = LicenseFileName();
	//
	QFile file(strLicenseFile);
	file.open(QIODevice::ReadWrite);

	QDataStream inOut(&file);

	//读取许可文件头部信息
	{
		//获取魔术数字
		qint64 magicNumber;
		inOut >> magicNumber;

		//获取qt版本信息
		qint64 version;
		inOut >> version;

		inOut.setVersion(version);
	}

	//判断所有产品的许可
	while (!inOut.atEnd())
	{
		//产品名称
		QString strName;
		inOut >> strName;

		//产品版本
		QString strVersion;
		inOut >> strVersion;

		//第一次运行时间
		qint64 startDays;
		inOut >> startDays;

		//许可到期时间
		qint64 endDays;
		inOut >> endDays;

		//可使用时间的位置
		qint64 position = inOut.device()->pos();
		//可试用时间
		qint64 validDays;
		inOut >> validDays;

		QDate srcDate = QDate(1900, 1, 1);
		//如果许可到期，则将许可状态置为无效
		if (srcDate.addDays(endDays) < QDate::currentDate()
			|| validDays == 0)
		{
			inOut << (qint64)eInvalid;
			m_licenseState = eInvalid;
		}
		else
		{
			//计算当前使用天数
			qint64 days = endDays - srcDate.daysTo(QDate::currentDate());
			//如果当前使用天数不高于许可文件的天数，则更新许可文件中的天数
			if (days <= validDays)
			{
				inOut.device()->seek(position);
				inOut << days;
				//许可状态
				qint64 state;
				inOut >> state;
				m_licenseState = (LicenseState)state;
			}
			else
			{
				inOut.device()->seek(position);
				inOut << 0;
				inOut << (qint64)eInvalid;
				m_licenseState = eInvalid;
			}
		}
		

		//如果找到相应的应用程序的许可信息，则获取并返回
		if (strName == m_applicationName && strVersion == m_applicationVersion)
		{
			return true;
		}
	}

	file.close();

	return false;
}

//激活许可
void LicenseFile::Active(ActiveMode mode)
{
	//获取许可文件名
	const QString strLicenseFile = s_instance.LicenseFileName();
	//
	QFile file(strLicenseFile);
	file.open(QIODevice::ReadWrite);

	QDataStream inOut(&file);

	//读取许可文件头部信息
	{
		//获取魔术数字
		qint64 magicNumber;
		inOut >> magicNumber;

		//获取qt版本信息
		qint64 version;
		inOut >> version;

		inOut.setVersion(version);
	}


	//判断所有产品的许可
	while (!inOut.atEnd())
	{
		//产品名称
		QString strName;
		inOut >> strName;

		//产品版本
		QString strVersion;
		inOut >> strVersion;

		//第一次运行时间
		qint64 startDays;
		inOut >> startDays;

		//如果找到相应的应用程序的许可信息，激活
		if (strName == s_instance.m_applicationName 
			&& strVersion == s_instance.m_applicationVersion)
		{
			if (mode == eOneTime)
			{
				qint64 endDays;
				inOut >> endDays;

				qint64 validDays;
				inOut >> validDays;
			}
			else if (mode == ePerYear)
			{
				QDate srcDate = QDate(1900, 1, 1);
				//许可有效日期
				inOut << (qint64)srcDate.daysTo(QDate::currentDate().addYears(1));
				int daysOneyear = (qint64)srcDate.daysTo(QDate::currentDate().addYears(1)) - (qint64)srcDate.daysTo(QDate::currentDate());
				inOut << (qint64)daysOneyear;
			}

			//许可状态
			inOut << (qint64)eActivate;
		}
		else
		{
			qint64 endDays;
			inOut >> endDays;

			qint64 validDays;
			inOut >> validDays;

			qint64 licenseState;
			inOut >> licenseState;
		}
	}

	file.close();
}

//激活许可
void LicenseFile::UnActive()
{
	//获取许可文件名
	const QString strLicenseFile = s_instance.LicenseFileName();
	//
	QFile file(strLicenseFile);
	file.open(QIODevice::ReadWrite);

	QDataStream inOut(&file);

	//读取许可文件头部信息
	{
		//获取魔术数字
		qint64 magicNumber;
		inOut >> magicNumber;

		//获取qt版本信息
		qint64 version;
		inOut >> version;

		inOut.setVersion(version);
	}


	//判断所有产品的许可
	while (!inOut.atEnd())
	{
		//产品名称
		QString strName;
		inOut >> strName;

		//产品版本
		QString strVersion;
		inOut >> strVersion;

		//第一次运行时间
		qint64 startDays;
		inOut >> startDays;

		//如果找到相应的应用程序的许可信息，激活
		if (strName == s_instance.m_applicationName 
			&& strVersion == s_instance.m_applicationVersion)
		{
			//许可有效日期
			inOut << (qint64)0;
			inOut << (qint64)0;
			//许可状态
			inOut << (qint64)eInvalid;
		}
		else
		{
			qint64 endDays;
			inOut >> endDays;

			qint64 validDays;
			inOut >> validDays;

			qint64 licenseState;
			inOut >> licenseState;
		}
	}

	file.close();
}

//剩余试用天数
int LicenseFile::ValidDaysOfTrial()
{
	//许可状态
	LicenseState state = s_instance.GetLicenseState();

	//如果许可无效，返回0
	if (state == eInvalid)
	{
		return 0;
	}
	else if (state == eTrial)
	{
		int daysOfTrial = 0;

		//获取许可文件名
		const QString strLicenseFile = s_instance.LicenseFileName();
		//
		QFile file(strLicenseFile);
		file.open(QIODevice::ReadWrite);

		QDataStream in(&file);

		//读取许可文件头部信息
		{
			//获取魔术数字
			qint64 magicNumber;
			in >> magicNumber;

			//获取qt版本信息
			qint64 version;
			in >> version;

			in.setVersion(version);
		}

		//判断所有产品的许可
		while (!in.atEnd())
		{
			//产品名称
			QString strName;
			in >> strName;

			//产品版本
			QString strVersion;
			in >> strVersion;

			//第一次运行时间
			qint64 startDays;
			in >> startDays;

			//许可到期时间
			qint64 endDays;
			in >> endDays;

			//有效使用天数
			qint64 validDays;
			in >> validDays;

			//许可状态
			qint64 state;
			in >> state;


			//如果找到相应的应用程序的许可信息，则获取并返回
			if (strName == s_instance.m_applicationName 
				&& strVersion == s_instance.m_applicationVersion)
			{
// 				QDate srcDate = QDate(1900, 1, 1);
// 				daysOfTrial = endDays - srcDate.daysTo(QDate::currentDate());
// 				if (daysOfTrial != validDays)
// 				{
// 					daysOfTrial = 0;
// 				}
				daysOfTrial = validDays;
				break;
			}
		}

		file.close();
		return daysOfTrial;
	}

	return -1;
}

//获取机器码
QString LicenseFile::MachineCode()
{
	//硬件信息
	const QString strHardInfo = HardwareInof::MachineCode();

	//对软件信息进行加密
	QString strSoftInfo = s_instance.m_organizationName
		+ s_instance.m_applicationName + s_instance.m_applicationVersion;

	QByteArray byteArrayTemp;
	byteArrayTemp = QCryptographicHash::hash(strSoftInfo.toUtf8(),
		QCryptographicHash::Md5);
	strSoftInfo = byteArrayTemp.toHex();
	strSoftInfo = strSoftInfo.toUpper();

	//生成机器码
	QString strMachineCode = strHardInfo + strSoftInfo;
	byteArrayTemp = QCryptographicHash::hash(strMachineCode.toUtf8(),
		QCryptographicHash::Md5);
	strMachineCode = byteArrayTemp.toHex();
	strMachineCode = strMachineCode.toUpper();

	//返回机器码
	return strMachineCode;
}