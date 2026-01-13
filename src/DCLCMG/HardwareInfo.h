#ifndef DC_LICENSEMANAGER_HARDWAREINFO_H
#define DC_LICENSEMANAGER_HARDWAREINFO_H

//Qt
class QString;

namespace DcLCMG
{
	class HardwareInof
	{
	public:
		static QString MachineCode();

	private:
		//主板信息
		QString InfosOfBIOS();

		//获取CPU信息
		QString InfosOfCPU();

		//硬盘信息
		QString InfosOfHardDisk();

	private:
		static HardwareInof s_instance;
	};
}

#endif