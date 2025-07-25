#define LOG_MODULE CommonLogModuleIpUtils

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <bitset>
#include "Logger.h"
#include "IpUtils.h"
#include "IpAddress.h"
#include "EndianPortable.h"

// for AF_INET, AF_INET6
#if !defined(_WIN32)
#	include <sys/socket.h>
#endif

namespace pcpp
{

	const IPv4Address IPv4Address::Zero;
	const IPv6Address IPv6Address::Zero;

	const IPv4Address IPv4Address::MulticastRangeLowerBound("224.0.0.0");
	const IPv4Address IPv4Address::MulticastRangeUpperBound("239.255.255.255");
	const IPv6Address IPv6Address::MulticastRangeLowerBound("ff00:0000:0000:0000:0000:0000:0000:0000");

	// ~~~~~~~~~~~
	// IPv4Address
	// ~~~~~~~~~~~

	std::string IPv4Address::toString() const
	{
		char addrBuffer[INET_ADDRSTRLEN];

		if (inet_ntop(AF_INET, toBytes(), addrBuffer, sizeof(addrBuffer)) != nullptr)
		{
			return addrBuffer;
		}

		return {};
	}

	bool IPv4Address::isMulticast() const
	{
		return !operator<(MulticastRangeLowerBound) &&
		       (operator<(MulticastRangeUpperBound) || operator==(MulticastRangeUpperBound));
	}

	IPv4Address::IPv4Address(const uint8_t* bytes, size_t size)
	{
		if (bytes == nullptr)
		{
			throw std::invalid_argument("Buffer pointer is null");
		}

		if (size < 4)
		{
			throw std::out_of_range("Buffer size is smaller than IPv4 address size");
		}
		memcpy(m_Bytes.data(), bytes, 4 * sizeof(uint8_t));
	}

	IPv4Address::IPv4Address(const std::string& addrAsString)
	{
		if (inet_pton(AF_INET, addrAsString.data(), m_Bytes.data()) <= 0)
		{
			throw std::invalid_argument("Not a valid IPv4 address: " + addrAsString);
		}
	}

	bool IPv4Address::matchNetwork(const IPv4Network& network) const
	{
		return network.includes(*this);
	}

	bool IPv4Address::matchNetwork(const std::string& network) const
	{
		try
		{
			auto ipv4Network = IPv4Network(network);
			return ipv4Network.includes(*this);
		}
		catch (const std::invalid_argument& e)
		{
			(void)e;  // Suppress the unreferenced local variable warning when PCPP_LOG_ERROR is disabled
			PCPP_LOG_ERROR(e.what());
			return false;
		}
	}

	bool IPv4Address::isValidIPv4Address(const std::string& addrAsString)
	{
		sockaddr_in sa_in{};
		return inet_pton(AF_INET, addrAsString.data(), &(sa_in.sin_addr)) > 0;
	}

	// ~~~~~~~~~~~
	// IPv6Address
	// ~~~~~~~~~~~

	std::string IPv6Address::toString() const
	{
		char addrBuffer[INET6_ADDRSTRLEN];

		if (inet_ntop(AF_INET6, toBytes(), addrBuffer, sizeof(addrBuffer)) != nullptr)
		{
			return addrBuffer;
		}

		return {};
	}

	bool IPv6Address::isMulticast() const
	{
		return !operator<(MulticastRangeLowerBound);
	}

	IPv6Address::IPv6Address(const uint8_t* bytes, size_t size)
	{
		if (bytes == nullptr)
		{
			throw std::invalid_argument("Buffer pointer is null");
		}

		if (size < 16)
		{
			throw std::out_of_range("Buffer size is smaller than IPv6 address size");
		}
		std::memcpy(m_Bytes.data(), bytes, 16 * sizeof(uint8_t));
	}

	IPv6Address::IPv6Address(const std::string& addrAsString)
	{
		if (inet_pton(AF_INET6, addrAsString.data(), m_Bytes.data()) <= 0)
		{
			throw std::invalid_argument("Not a valid IPv6 address: " + addrAsString);
		}
	}

	void IPv6Address::copyTo(uint8_t** arr, size_t& length) const
	{
		const size_t addrLen = m_Bytes.size() * sizeof(uint8_t);
		length = addrLen;
		*arr = new uint8_t[addrLen];
		memcpy(*arr, m_Bytes.data(), addrLen);
	}

	size_t IPv6Address::copyTo(uint8_t* buffer, size_t size) const
	{
		const size_t requiredSize = m_Bytes.size();

		if (buffer == nullptr)
		{
			if (size != 0)
			{
				throw std::invalid_argument("Buffer is null but size is not zero");
			}

			return requiredSize;
		}

		if (size < requiredSize)
		{
			return requiredSize;
		}

		std::memcpy(buffer, m_Bytes.data(), requiredSize);
		return requiredSize;
	}

	bool IPv6Address::copyToNewBuffer(uint8_t** buffer, size_t& size) const
	{
		if (buffer == nullptr)
		{
			throw std::invalid_argument("Buffer pointer is null");
		}

		size = copyTo(nullptr, 0);
		*buffer = new uint8_t[size];
		if (copyTo(*buffer, size) != size)
		{
			delete[] *buffer;
			*buffer = nullptr;
			size = 0;
			return false;
		}

		return true;
	}

	bool IPv6Address::matchNetwork(const IPv6Network& network) const
	{
		return network.includes(*this);
	}

	bool IPv6Address::matchNetwork(const std::string& network) const
	{
		try
		{
			auto ipv6Network = IPv6Network(network);
			return ipv6Network.includes(*this);
		}
		catch (const std::invalid_argument& e)
		{
			(void)e;  // Suppress the unreferenced local variable warning when PCPP_LOG_ERROR is disabled
			PCPP_LOG_ERROR(e.what());
			return false;
		}
	}

	bool IPv6Address::isValidIPv6Address(const std::string& addrAsString)
	{
		sockaddr_in6 sa_in6{};
		return inet_pton(AF_INET6, addrAsString.data(), &(sa_in6.sin6_addr)) > 0;
	}

	// ~~~~~~~~~
	// IPAddress
	// ~~~~~~~~~

	IPAddress::IPAddress(const std::string& addrAsString)
	{
		if (IPv4Address::isValidIPv4Address(addrAsString))
		{
			m_Type = IPv4AddressType;
			m_IPv4 = IPv4Address(addrAsString);
		}
		else if (IPv6Address::isValidIPv6Address(addrAsString))
		{
			m_Type = IPv6AddressType;
			m_IPv6 = IPv6Address(addrAsString);
		}
		else
		{
			throw std::invalid_argument("Not a valid IP address: " + addrAsString);
		}
	}

	// ~~~~~~~~~~~
	// IPv4Network
	// ~~~~~~~~~~~

	bool IPv4Network::isValidNetmask(const IPv4Address& maskAddress)
	{
		if (maskAddress == IPv4Address::Zero)
		{
			return true;
		}

		const uint32_t maskAsInt = be32toh(maskAddress.toInt());
		const std::bitset<32> bitset(maskAsInt);
		auto bitsetCount = bitset.count();

		if (bitsetCount == 32)
		{
			return true;
		}

		return maskAsInt << bitsetCount == 0;
	}

	void IPv4Network::initFromAddressAndPrefixLength(const IPv4Address& address, uint8_t prefixLen)
	{
		m_Mask = be32toh(0xffff'ffff ^ (prefixLen < 32 ? 0xffff'ffff >> prefixLen : 0));
		m_NetworkPrefix = address.toInt() & m_Mask;
	}

	void IPv4Network::initFromAddressAndNetmask(const IPv4Address& address, const IPv4Address& netmaskAddress)
	{
		m_Mask = netmaskAddress.toInt();
		m_NetworkPrefix = address.toInt() & m_Mask;
	}

	IPv4Network::IPv4Network(const IPv4Address& address, uint8_t prefixLen)
	{
		if (prefixLen > 32)
		{
			throw std::invalid_argument("prefixLen must be an integer between 0 and 32");
		}

		initFromAddressAndPrefixLength(address, prefixLen);
	}

	IPv4Network::IPv4Network(const IPv4Address& address, const std::string& netmask)
	{
		IPv4Address netmaskAddr;
		try
		{
			netmaskAddr = IPv4Address(netmask);
		}
		catch (const std::exception&)
		{
			throw std::invalid_argument("Netmask is not valid IPv4 format: " + netmask);
		}
		if (!isValidNetmask(netmaskAddr))
		{
			throw std::invalid_argument("Netmask is not valid IPv4 format: " + netmask);
		}
		initFromAddressAndNetmask(address, netmaskAddr);
	}

	IPv4Network::IPv4Network(const std::string& addressAndNetmask)
	{
		std::stringstream stream(addressAndNetmask);
		std::string networkPrefixStr;
		std::string netmaskStr;
		std::getline(stream, networkPrefixStr, '/');
		std::getline(stream, netmaskStr);

		if (netmaskStr.empty())
		{
			throw std::invalid_argument(
			    "The input should be in the format of <address>/<netmask> or <address>/<prefixLength>");
		}

		IPv4Address networkPrefix;
		try
		{
			networkPrefix = IPv4Address(networkPrefixStr);
		}
		catch (const std::invalid_argument&)
		{
			throw std::invalid_argument("The input doesn't contain a valid IPv4 network prefix: " + networkPrefixStr);
		}

		if (std::all_of(netmaskStr.begin(), netmaskStr.end(), ::isdigit))
		{
			const uint32_t prefixLen = std::stoi(netmaskStr);
			if (prefixLen > 32)
			{
				throw std::invalid_argument("Prefix length must be an integer between 0 and 32");
			}

			initFromAddressAndPrefixLength(networkPrefix, prefixLen);
		}
		else
		{
			IPv4Address netmaskAddr;
			try
			{
				netmaskAddr = IPv4Address(netmaskStr);
			}
			catch (const std::invalid_argument&)
			{
				throw std::invalid_argument("Netmask is not valid IPv4 format: " + netmaskStr);
			}
			if (!isValidNetmask(netmaskAddr))
			{
				throw std::invalid_argument("Netmask is not valid IPv4 format: " + netmaskStr);
			}
			initFromAddressAndNetmask(networkPrefix, netmaskAddr);
		}
	}

	uint8_t IPv4Network::getPrefixLen() const
	{
		const std::bitset<32> bitset(m_Mask);
		return bitset.count();
	}

	IPv4Address IPv4Network::getLowestAddress() const
	{
		const std::bitset<32> bitset(m_Mask);
		return bitset.count() < 32 ? m_NetworkPrefix + htobe32(1) : m_NetworkPrefix;
	}

	IPv4Address IPv4Network::getHighestAddress() const
	{
		auto tempAddress = static_cast<uint32_t>(m_NetworkPrefix | ~m_Mask);
		const std::bitset<32> bitset(m_Mask);
		return bitset.count() < 32 ? tempAddress - htobe32(1) : tempAddress;
	}

	uint64_t IPv4Network::getTotalAddressCount() const
	{
		const std::bitset<32> bitset(~static_cast<uint64_t>(m_Mask));
		return 1ULL << bitset.count();
	}

	bool IPv4Network::includes(const IPv4Address& address) const
	{
		return (address.toInt() & m_Mask) == m_NetworkPrefix;
	}

	bool IPv4Network::includes(const IPv4Network& network) const
	{
		const uint32_t lowestAddress = network.m_NetworkPrefix;
		const uint32_t highestAddress = network.m_NetworkPrefix | ~network.m_Mask;
		return ((lowestAddress & m_Mask) == m_NetworkPrefix && (highestAddress & m_Mask) == m_NetworkPrefix);
	}

	std::string IPv4Network::toString() const
	{
		std::ostringstream stream;
		stream << getNetworkPrefix() << "/" << static_cast<int>(getPrefixLen());
		return stream.str();
	}

	// ~~~~~~~~~~~
	// IPv6Network
	// ~~~~~~~~~~~

#define IPV6_ADDR_SIZE 16

	bool IPv6Network::isValidNetmask(const IPv6Address& netmask)
	{
		if (netmask == IPv6Address::Zero)
		{
			return true;
		}

		const uint8_t* addressAsBytes = netmask.toBytes();
		int expectingValue = 1;
		for (auto byteIndex = 0; byteIndex < IPV6_ADDR_SIZE; byteIndex++)
		{
			auto curByte = addressAsBytes[byteIndex];
			if (expectingValue == 1)
			{
				if (curByte == 0xff)
				{
					continue;
				}
				const std::bitset<8> bitset(curByte);
				if (((curByte << bitset.count()) & 0xff) != 0)
				{
					return false;
				}
				expectingValue = 0;
			}
			else if (expectingValue == 0 && curByte != 0)
			{
				return false;
			}
		}

		return true;
	}

	void IPv6Network::initFromAddressAndPrefixLength(const IPv6Address& address, uint8_t prefixLen)
	{
		memset(m_Mask, 0, IPV6_ADDR_SIZE);
		int remainingPrefixLen = prefixLen;
		for (auto& byte : m_Mask)
		{
			if (remainingPrefixLen >= 8)
			{
				byte = 0xff;
			}
			else if (remainingPrefixLen > 0)
			{
				byte = 0xff << (8 - remainingPrefixLen);
			}
			else
			{
				break;
			}

			remainingPrefixLen -= 8;
		}

		address.copyTo(m_NetworkPrefix);

		for (auto byteIndex = 0; byteIndex < IPV6_ADDR_SIZE; byteIndex++)
		{
			m_NetworkPrefix[byteIndex] &= m_Mask[byteIndex];
		}
	}

	void IPv6Network::initFromAddressAndNetmask(const IPv6Address& address, const IPv6Address& netmaskAddr)
	{
		netmaskAddr.copyTo(m_Mask);

		address.copyTo(m_NetworkPrefix);

		for (auto byteIndex = 0; byteIndex < IPV6_ADDR_SIZE; byteIndex++)
		{
			m_NetworkPrefix[byteIndex] &= m_Mask[byteIndex];
		}
	}

	IPv6Network::IPv6Network(const IPv6Address& address, uint8_t prefixLen)
	{
		if (prefixLen > 128)
		{
			throw std::invalid_argument("prefixLen must be an integer between 0 and 128");
		}

		initFromAddressAndPrefixLength(address, prefixLen);
	}

	IPv6Network::IPv6Network(const IPv6Address& address, const std::string& netmask)
	{
		IPv6Address netmaskAddr;
		try
		{
			netmaskAddr = IPv6Address(netmask);
		}
		catch (const std::exception&)
		{
			throw std::invalid_argument("Netmask is not valid IPv6 format: " + netmask);
		}
		if (!isValidNetmask(netmaskAddr))
		{
			throw std::invalid_argument("Netmask is not valid IPv6 format: " + netmask);
		}
		initFromAddressAndNetmask(address, netmaskAddr);
	}

	IPv6Network::IPv6Network(const std::string& addressAndNetmask)
	{
		std::stringstream stream(addressAndNetmask);
		std::string networkPrefixStr;
		std::string netmaskStr;
		std::getline(stream, networkPrefixStr, '/');
		std::getline(stream, netmaskStr);

		if (netmaskStr.empty())
		{
			throw std::invalid_argument(
			    "The input should be in the format of <address>/<netmask> or <address>/<prefixLength>");
		}

		IPv6Address networkPrefix;
		try
		{
			networkPrefix = IPv6Address(networkPrefixStr);
		}
		catch (const std::invalid_argument&)
		{
			throw std::invalid_argument("The input doesn't contain a valid IPv6 network prefix: " + networkPrefixStr);
		}
		if (std::all_of(netmaskStr.begin(), netmaskStr.end(), ::isdigit))
		{
			const uint32_t prefixLen = std::stoi(netmaskStr);
			if (prefixLen > 128)
			{
				throw std::invalid_argument("Prefix length must be an integer between 0 and 128");
			}

			initFromAddressAndPrefixLength(networkPrefix, prefixLen);
		}
		else
		{
			IPv6Address netmaskAddr;
			try
			{
				netmaskAddr = IPv6Address(netmaskStr);
			}
			catch (const std::exception&)
			{
				throw std::invalid_argument("Netmask is not valid IPv6 format: " + netmaskStr);
			}
			if (!isValidNetmask(netmaskAddr))
			{
				throw std::invalid_argument("Netmask is not valid IPv6 format: " + netmaskStr);
			}
			initFromAddressAndNetmask(networkPrefix, netmaskAddr);
		}
	}

	uint8_t IPv6Network::getPrefixLen() const
	{
		uint8_t result = 0;
		for (const auto& byte : m_Mask)
		{
			const std::bitset<8> bset(byte);
			result += static_cast<uint8_t>(bset.count());
		}
		return result;
	}

	IPv6Address IPv6Network::getLowestAddress() const
	{
		if (getPrefixLen() == 128)
		{
			return m_NetworkPrefix;
		}

		uint8_t lowestAddress[IPV6_ADDR_SIZE];
		memcpy(lowestAddress, m_NetworkPrefix, IPV6_ADDR_SIZE);
		lowestAddress[IPV6_ADDR_SIZE - 1]++;
		return lowestAddress;
	}

	IPv6Address IPv6Network::getHighestAddress() const
	{
		uint8_t result[IPV6_ADDR_SIZE];

		for (auto byteIndex = 0; byteIndex < IPV6_ADDR_SIZE; byteIndex++)
		{
			result[byteIndex] = m_NetworkPrefix[byteIndex] | ~m_Mask[byteIndex];
		}

		return result;
	}

	uint64_t IPv6Network::getTotalAddressCount() const
	{
		int numOfBitset = 0;
		for (const auto& byte : m_Mask)
		{
			const std::bitset<8> bitset(static_cast<uint8_t>(~byte));
			numOfBitset += static_cast<int>(bitset.count());
		}

		if (numOfBitset >= 64)
		{
			throw std::out_of_range("Number of addresses exceeds uint64_t");
		}
		return 1ULL << numOfBitset;
	}

	bool IPv6Network::includes(const IPv6Address& address) const
	{
		uint8_t maskedBytes[IPV6_ADDR_SIZE];
		address.copyTo(maskedBytes);

		for (auto byteIndex = 0; byteIndex < IPV6_ADDR_SIZE; byteIndex++)
		{
			maskedBytes[byteIndex] &= m_Mask[byteIndex];
		}
		return memcmp(m_NetworkPrefix, maskedBytes, IPV6_ADDR_SIZE) == 0;
	}

	bool IPv6Network::includes(const IPv6Network& network) const
	{
		return includes(network.getLowestAddress()) && includes(network.getHighestAddress());
	}

	std::string IPv6Network::toString() const
	{
		std::ostringstream stream;
		stream << getNetworkPrefix() << "/" << static_cast<int>(getPrefixLen());
		return stream.str();
	}

}  // namespace pcpp
