#pragma once

/// @file

#include "PcapLiveDevice.h"

/// @namespace pcpp
/// @brief The main namespace for the PcapPlusPlus lib
namespace pcpp
{
	/// @class WinPcapLiveDevice
	/// A class that wraps a Windows network interface (each of the interfaces listed in ipconfig).
	/// This class is almost similar in its capabilities to PcapLiveDevice (its parent class) with some small changes
	/// that mainly result from differences between libpcap and WinPcap/Npcap. Please see the reference for
	/// PcapLiveDevice for more details
	class WinPcapLiveDevice : public PcapLiveDevice
	{
		friend class PcapLiveDeviceList;

	protected:
		int m_MinAmountOfDataToCopyFromKernelToApplication;

		// c'tor is not public, there should be only one for every interface (created by PcapLiveDeviceList)
		WinPcapLiveDevice(pcap_if_t* iface, bool calculateMTU, bool calculateMacAddress, bool calculateDefaultGateway)
		    : WinPcapLiveDevice(DeviceInterfaceDetails(iface), calculateMTU, calculateMacAddress,
		                        calculateDefaultGateway)
		{}
		WinPcapLiveDevice(DeviceInterfaceDetails interfaceDetails, bool calculateMTU, bool calculateMacAddress,
		                  bool calculateDefaultGateway);

	public:
		WinPcapLiveDevice(const WinPcapLiveDevice& other) = delete;
		WinPcapLiveDevice& operator=(const WinPcapLiveDevice& other) = delete;

		LiveDeviceType getDeviceType() const override
		{
			return WinPcapDevice;
		}

		using PcapLiveDevice::sendPackets;
		virtual int sendPackets(RawPacket* rawPacketsArr, int arrLength);

		/// WinPcap/Npcap have a feature (that doesn't exist in libpcap) to change the minimum amount of data in the
		/// kernel buffer that causes a read from the application to return (unless the timeout expires). Please see
		/// documentation for pcap_setmintocopy for more info. This method enables the user to change this size. Note
		/// the device must be open for this method to work
		/// @param[in] size The size to set in bytes
		/// @return True if set succeeded, false if the device is closed or if pcap_setmintocopy failed
		bool setMinAmountOfDataToCopyFromKernelToApplication(int size);

		/// @return The current amount of data in the kernel buffer that causes a read from the application to return
		/// (see also setMinAmountOfDataToCopyFromKernelToApplication())
		int getMinAmountOfDataToCopyFromKernelToApplication() const
		{
			return m_MinAmountOfDataToCopyFromKernelToApplication;
		}

		WinPcapLiveDevice* clone() const override;

	protected:
		void prepareCapture(bool asyncCapture, bool captureStats) override;
	};
}  // namespace pcpp
