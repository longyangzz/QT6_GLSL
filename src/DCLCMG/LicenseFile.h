#ifndef DC_LICENSEMANAGER_LICENSEFILE_H
#define DC_LICENSEMANAGER_LICENSEFILE_H

//qt
#include "QString"

#include "LicenseManager.h"

namespace DcLCMG
{
	class LicenseFile
	{
	public:
		LicenseFile(); 

		//返回证书的许可状态
		static int StateOfLicense();

		//设置公司名称
		static void SetOrganizationName(const QString& strOrganizationName);

		//设置应用程序名称
		static void SetApplicationName(const QString& strApplicationName);

		//设置应用程序版本信息
		static void SetApplicationVersion(const QString& strApplicationVersion);

		//设置试用天数
		static void SetTrialDays(unsigned days);

		//获取机器码
		static QString MachineCode();

		//获取剩余试用天数
		static int ValidDaysOfTrial();

		//激活许可
		static void Active(ActiveMode mode);

		//取消激活
		static void UnActive();
		
	private:
		//许可文件是否存在
		bool IsExist();

		//创建许可文件
		//许可文件创建成功返回true，否则返回false
		void Init();

		//许可文件名
		QString LicenseFileName();

		//获取许可状态
		LicenseState GetLicenseState();

		//文件中是否存在许可
		bool ExistLicenseInFile();

	private:
		static LicenseFile s_instance;

		//公司名称
		QString m_organizationName;

		//应用程序名称
		QString m_applicationName;

		//应用程序版本
		QString m_applicationVersion;

		//许可文件名称
		QString m_licenseFileName;

		//试用天数
		unsigned m_daysOfTrial;

		//许可状态
		LicenseState m_licenseState;

		//许可不存在的状态
		enum ExistType
		{
			eExist				=	0x11111111	,	//许可存在
			eFileNotExist		=	0x00000000	,	//许可文件不存在
			eLicenseNotExist	=	0x11110000		//许可不存在
		};
		ExistType m_existType;
	};
}

#endif