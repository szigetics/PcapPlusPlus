#pragma once

#include <vector>
#include <memory>
#include "PcapLiveDevice.h"

/// @file

struct pcap_rmtauth;

/// @namespace pcpp
/// @brief The main namespace for the PcapPlusPlus lib
namespace pcpp
{
	/// @struct PcapRemoteAuthentication
	/// The remote daemon (rpcapd) can be configured to require authentication before allowing a client to connect. This
	/// is done for security reasons of course. This struct wraps the WinPcap/Npcap authentication object (pcap_rmtauth)
	/// and can (but not must) be given to PcapRemoteDeviceList when initiating a connection to the remote daemon
	struct PcapRemoteAuthentication
	{
	public:
		/// A constructor that sets username and password
		/// @param[in] username The username for authentication with the remote daemon
		/// @param[in] password The password for authentication with the remote daemon
		PcapRemoteAuthentication(const std::string& username, const std::string& password) : userName(username)
		{
			this->password = password;
		}

		/// A copy c'tor for this object
		/// @param[in] other The object to copy from
		PcapRemoteAuthentication(const PcapRemoteAuthentication& other)
		    : userName(other.userName), password(other.password)
		{}

		/// The username for authentication
		std::string userName;

		/// The password for authentication
		std::string password;

		/// A conversion method from PcapRemoteAuthentication to pcap_rmtauth. Note: the char* pointers of the returned
		/// pcap_rmtauth points to the same places in memory as PcapRemoteAuthentication::userName and
		/// PcapRemoteAuthentication::password so the user should avoid freeing this memory
		/// @return A pcap_rmtauth that is converted from this class
		pcap_rmtauth getPcapRmAuth() const;
	};

	/// @class PcapRemoteDevice
	/// A class that provides a C++ wrapper for WinPcap/Npcap Remote Capture feature. This feature allows to interact to
	/// a remote machine and capture packets that are being transmitted on the remote network interfaces. This requires
	/// a remote daemon (called rpcapd) which performs the capture and sends data back and the local client (represented
	/// by PcapRemoteDevice) that sends the appropriate commands and receives the captured data. You can read more about
	/// this feature in WinPcap Remote Capture manual: https://www.winpcap.org/docs/docs_412/html/group__remote.html<BR>
	/// Since this feature is supported in WinPcap and Npcap only and not in libpcap, PcapRemoteDevice can only be used
	/// in Windows only.<BR> This class provides a wrapper for the local client, meaning it assumes the daemon (rpcapd)
	/// is already running on the remote machine and it tries to connect to it and start receiving/sending packets
	/// from/to it. This class assumes rpcapd is in passive mode, meaning PcapRemoteDevice connects to the remote
	/// daemon, sends the appropriate commands to it, and starts capturing packets, rather than letting the daemon
	/// connect to the client by itself. Using PcapRemoteDevice is very similar to using the other live devices
	/// (PcapLiveDevice or WinPcapLiveDevice), meaning the API's are the same and the same logic is used (for example:
	/// capturing is done on a different thread, sending packets are done on the same thread, etc.). For the full API
	/// and explanations, please refer to PcapLiveDevice. The reason for the similar API is that WinPcap/Npcap's API is
	/// very similar between Remote Capture and local network interface capture. The things that are different are some
	/// are some implementation details, mainly in making the connection to the remote daemon, and the way the user can
	/// get the instance of PcapRemoteDevice. For more details on that please refer to PcapRemoteDeviceList
	class PcapRemoteDevice : public PcapLiveDevice
	{
		friend class PcapRemoteDeviceList;

	private:
		IPAddress m_RemoteMachineIpAddress;
		uint16_t m_RemoteMachinePort;
		std::shared_ptr<PcapRemoteAuthentication> m_RemoteAuthentication;

		// c'tor is private, as only PcapRemoteDeviceList should create instances of it, and it'll create only one for
		// every remote interface
		PcapRemoteDevice(pcap_if_t* iface, std::shared_ptr<PcapRemoteAuthentication> remoteAuthentication,
		                 const IPAddress& remoteMachineIP, uint16_t remoteMachinePort)
		    : PcapRemoteDevice(DeviceInterfaceDetails(iface), std::move(remoteAuthentication), remoteMachineIP,
		                       remoteMachinePort)
		{}
		PcapRemoteDevice(DeviceInterfaceDetails deviceInterface,
		                 std::shared_ptr<PcapRemoteAuthentication> remoteAuthentication,
		                 const IPAddress& remoteMachineIP, uint16_t remoteMachinePort);

	public:
		PcapRemoteDevice(const PcapRemoteDevice&) = delete;
		PcapRemoteDevice(PcapRemoteDevice&&) noexcept = delete;
		PcapRemoteDevice& operator=(const PcapRemoteDevice&) = delete;
		PcapRemoteDevice& operator=(PcapRemoteDevice&&) noexcept = delete;

		~PcapRemoteDevice() override = default;

		/// @return The IP address of the remote machine where packets are transmitted from the remote machine to the
		/// client machine
		IPAddress getRemoteMachineIpAddress() const
		{
			return m_RemoteMachineIpAddress;
		}

		/// @return The port of the remote machine where packets are transmitted from the remote machine to the client
		/// machine
		uint16_t getRemoteMachinePort() const
		{
			return m_RemoteMachinePort;
		}

		/// @return The type of the device (libPcap, WinPcap/Npcap or a remote device)
		LiveDeviceType getDeviceType() const override
		{
			return RemoteDevice;
		}

		/// MTU isn't supported for remote devices
		/// @return 0
		uint32_t getMtu() const override;

		/// MAC address isn't supported for remote devices
		/// @return MacAddress#Zero
		MacAddress getMacAddress() const override;

		/// Open the device using pcap_open. Opening the device makes the connection to the remote daemon (including
		/// authentication if needed and provided). If this methods succeeds it means the connection to the remote
		/// daemon succeeded and the device is ready for use. As in PcapLiveDevice, packet capturing won't start yet.
		/// For packet capturing the user should call startCapture(). This implies that calling this method is a must
		/// before calling startCapture() (otherwise startCapture() will fail with a "device not open" error). The
		/// remote daemon is asked to capture packets in promiscuous mode
		/// @return True if the device was opened successfully, false otherwise. When opening the device fails an error
		/// will be printed to log as well, including the WinPcap/Npcap error if exists
		bool open() override;

		void getStatistics(IPcapDevice::PcapStats& stats) const override;

		PcapRemoteDevice* clone() const override;
	};
}  // namespace pcpp
