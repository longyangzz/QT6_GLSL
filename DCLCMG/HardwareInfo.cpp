#include "HardwareInfo.h"

using namespace DcLCMG;

//Qt
#include "QString"
#include "QCryptographicHash"
#include "QSettings"

HardwareInof HardwareInof::s_instance;

//机器码
QString HardwareInof::MachineCode()
{
	//主板信息
	QString strBIOSInfo = s_instance.InfosOfBIOS();

	//CPU信息
	QString strCPUInfo = s_instance.InfosOfCPU();

	//硬盘信息
	QString strHardDiskInfo = s_instance.InfosOfHardDisk();

	//未加密的机器码
	QString strMachineCode = strBIOSInfo + "---" + strCPUInfo + "---" + strHardDiskInfo;

	//对机器码使用MD5加密
	QByteArray byteArrayTemp = QCryptographicHash::hash(strMachineCode.toUtf8(),
		QCryptographicHash::Md5);
	strMachineCode = byteArrayTemp.toHex();

	//机器码转为大写字母，并返回
	return strMachineCode.toUpper();
}

//主板信息
QString HardwareInof::InfosOfBIOS()
{
	QString strBIOSInfo;

	//主板信息所在的注册表项
	QSettings settings("HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\BIOS",
		QSettings::NativeFormat);

	//临时字符串
	QString strTemp;
	//Baseboard信息
	{
		//生产商
		strTemp = settings.value("BaseBoardManufacturer").toString();
		strBIOSInfo.append(strTemp).append("-");
		//产品说明
		strTemp = settings.value("BaseBoardProduct").toString();
		strBIOSInfo.append(strTemp).append("-");
		//版本信息
		strTemp = settings.value("BaseBoardVersion").toString();
		strBIOSInfo.append(strTemp).append("-");
	}

	//BIOS信息
	{
		//销售商
		strTemp = settings.value("BIOSVendor").toString();
		strBIOSInfo.append(strTemp).append("-");
		//版本信息
		strTemp = settings.value("BIOSVersion").toString();
		strBIOSInfo.append(strTemp).append("-");
	}

	//System信息
	{
		//家族信息
		strTemp = settings.value("SystemFamily").toString();
		strBIOSInfo.append(strTemp).append("-");
		//生产商
		strTemp = settings.value("SystemManufacturer").toString();
		strBIOSInfo.append(strTemp).append("-");
		//产品名称
		strTemp = settings.value("SystemProductName").toString();
		strBIOSInfo.append(strTemp).append("-");
		//库存量单位
		strTemp = settings.value("SystemSKU").toString();
		strBIOSInfo.append(strTemp).append("-");
		//版本信息
		strTemp = settings.value("SystemVersion").toString();
		strBIOSInfo.append(strTemp);
	}

	return strBIOSInfo;
}

//CPU信息
QString HardwareInof::InfosOfCPU()
{
	QString strCPUInfo;

	//CPU所在的注册表项
	QSettings settings("HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
		QSettings::NativeFormat);
	QString strTemp;

	//标识号
	strTemp = settings.value("Identifier").toString();
	strCPUInfo.append(strTemp).append("-");
	//名字
	strTemp = settings.value("ProcessorNameString").toString();
	strCPUInfo.append(strTemp).append("-");
	//厂商标识号
	strTemp = settings.value("VendorIdentifier").toString();
	strCPUInfo.append(strTemp).append("-");
	//主频信息
	strTemp = QString::number(settings.value("~MHz").toULongLong());
	strCPUInfo.append(strTemp);
	

	return strCPUInfo;
}

//硬盘信息
QString HardwareInof::InfosOfHardDisk()
{
	QString strHardDiskInfo;

	//硬盘信息所在的注册表项
	QSettings setting("HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\"
		"MultifunctionAdapter\\0\\DiskController\\0\\DiskPeripheral\\0",
		QSettings::NativeFormat);

	//硬盘号
	strHardDiskInfo = setting.value("Identifier").toString();

	return strHardDiskInfo;
}