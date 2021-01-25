//Copyright 2020, ALT LLC. All Rights Reserved.
//This file is part of Antilatency SDK.
//It is subject to the license terms in the LICENSE file found in the top-level directory
//of this distribution and at http://www.antilatency.com/eula
//You may not use this file except in compliance with the License.
//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.
#pragma once
#ifndef ANTILATENCY_INTERFACE_CONTRACT_EXCEPTIONS_ENABLED
#define ANTILATENCY_INTERFACE_CONTRACT_EXCEPTIONS_ENABLED
#endif
#include <cstdint>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4946)
#pragma warning(disable: 4265)
#endif
namespace Antilatency {
	namespace RadioMetrics {
		namespace Interop {
			/// <summary>Extended metrics.</summary>
			struct ExtendedMetrics {
				/// <summary>Count of bytes, that was sent to targetNode.</summary>
				uint32_t txBytes;
				/// <summary>Count of packet, that was sent to targetNode.</summary>
				uint32_t txPacketsCount;
				/// <summary>Count of bytes, that was received from targetNode.</summary>
				uint32_t rxBytes;
				/// <summary>Count of packets, that was received from targetNode.</summary>
				uint32_t rxPacketsCount;
				/// <summary>Count of special empty packets. Should be 0. Indicator of problem with usb.</summary>
				uint32_t flowCount;
				/// <summary>Average rssi value in dBm.</summary>
				int8_t averageRssi;
				/// <summary>Min rssi value in dBm.(worse)</summary>
				int8_t minRssi;
				/// <summary>Max rssi value in dBm.(best)</summary>
				int8_t maxRssi;
				/// <summary>Count of packets that wasn't receive.</summary>
				uint32_t missedPacketsCount;
				/// <summary>Count of packets with error.</summary>
				uint32_t failedPacketsCount;
			};
		} //namespace Interop
	} //namespace RadioMetrics
} //namespace Antilatency


#ifdef _MSC_VER
#pragma warning(pop)
#endif
