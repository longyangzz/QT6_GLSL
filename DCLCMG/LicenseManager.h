/*****************************************************************************
使用许可管理器必须设置公司名称、应用程序名称、应用程序版本号
*****************************************************************************/
#ifndef DC_LICENSEMANAGER_LICENSEMANAGER_H
#define DC_LICENSEMANAGER_LICENSEMANAGER_H

#include "dclcmg_global.h"

//Qt
#include "QString"

namespace DcLCMG
{
	//!brief 证书许可状态
	enum LicenseState
	{
		eInvalid		=	0x00000000		,	//许可无效
		eTrial			=	0x00001111		,	//试用许可
		eActivate		=	0x11111111			//许可激活
	};

	//!brief 许可证书结构
	struct License
	{
		QString userName;						//用户名
		QString machineCode;					//机器码
		QString licenseCode;					//注册码
	};

	//!brief 激活模式
	enum ActiveMode
	{
		ePerYear			=	0x00001000	,	//每年一激活
		eOneTime			=	0x00002000		//一次性激活
	};

	//!brief 证书许可管理器
	class DCLCMG_EXPORT LicenseManager
	{
	public:
		//!brief 设置公司名称
		static void SetOrganizationName(const QString& strOrganizationName);

		//!brief 设置应用程序名称
		static void SetApplicationName(const QString& strApplicationName);

		//!brief 设置应用程序版本信息
		static void SetApplicationVersion(const QString& strApplicationVersion);

		//!brief 设置试用天数
		static void SetTrialDays(unsigned days);

		//!brief 证书许可状态
		//!retval 返回证书的许可状态
		static LicenseState StateOfLicense();

		//!brief 剩余试用天数
		//!retval 如果许可激活返回-1；试用许可返回试用天数；许可无效返回0
		static int ValidDaysOfTrial();

		//!brief 机器码
		//!retval 返回当前电脑的机器码
		static QString MachineCode();

		//!brief 激活许可
		//!brief 返回激活状态，成功返回true，失败返回false
		static bool ActiveLicense(License license, ActiveMode mode = ePerYear);

		//!brief 取消激活
		static void UnActiveLicense();
	};
}

#endif