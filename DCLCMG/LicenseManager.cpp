#include "LicenseManager.h"

using namespace DcLCMG;

//Qt
#include "QCryptographicHash"

#include "LicenseFile.h"

bool CheckLicense(License license)
{
	QByteArray byteArrayTemp;

	//对用户名进行加密
	QString strUserName = license.userName;

	byteArrayTemp = QCryptographicHash::hash(strUserName.toUtf8(),
		QCryptographicHash::Md5);
	strUserName = byteArrayTemp.toHex();
	strUserName = strUserName.toUpper();

	//组合用户名和机器码
	int len = strUserName.size();

	QString strLicenseCode;
	for (int i = 0; i < len; ++i)
	{
		strLicenseCode.append(strUserName[i]).append(license.machineCode[len - i - 1]);
	}

	//对注册码进行加密
	byteArrayTemp = QCryptographicHash::hash(strLicenseCode.toUtf8(),
		QCryptographicHash::Md5);
	strLicenseCode = byteArrayTemp.toHex();
	strLicenseCode = strLicenseCode.toUpper();

	//检验注册码的有效性
	return strLicenseCode == license.licenseCode;
}

//设置公司名称
void LicenseManager::SetOrganizationName(const QString& strOrganizationName)
{
	LicenseFile::SetOrganizationName(strOrganizationName);
}

//设置应用程序名称
void LicenseManager::SetApplicationName(const QString& strApplicationName)
{
	LicenseFile::SetApplicationName(strApplicationName);
}

//设置应用程序版本
void LicenseManager::SetApplicationVersion(const QString& strApplicationVersion)
{
	LicenseFile::SetApplicationVersion(strApplicationVersion);
}

//设置试用天数
void LicenseManager::SetTrialDays(unsigned days)
{
	LicenseFile::SetTrialDays(days);
}

//许可状态
LicenseState LicenseManager::StateOfLicense()
{
	return (LicenseState)LicenseFile::StateOfLicense();
}

//试用天数
int LicenseManager::ValidDaysOfTrial()
{
	return LicenseFile::ValidDaysOfTrial();
}

//机器码
QString LicenseManager::MachineCode()
{
	return LicenseFile::MachineCode();
}

//激活许可
bool LicenseManager::ActiveLicense(License license, ActiveMode mode)
{
	//检查许可证书
	bool bActiveState = CheckLicense(license);

	//如果许可证书有效，则激活
	if (bActiveState)
	{
		LicenseFile::Active(mode);
	}

	return bActiveState;
}

//取消激活
void LicenseManager::UnActiveLicense()
{
	LicenseFile::UnActive();
}