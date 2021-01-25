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
	namespace HardwareExtensionInterface {
		namespace Interop {
			/// <summary>Logical level on pin.</summary>
			enum class PinState : int32_t {
				Low = 0x0,
				High = 0x1
			};
		} //namespace Interop
	} //namespace HardwareExtensionInterface
} //namespace Antilatency
namespace Antilatency {
	inline const char* enumToString(const Antilatency::HardwareExtensionInterface::Interop::PinState& x) {
		switch (x) {
			case Antilatency::HardwareExtensionInterface::Interop::PinState::Low: return "Low";
			case Antilatency::HardwareExtensionInterface::Interop::PinState::High: return "High";
		}
		return "";
	}
} //namespace Antilatency

namespace Antilatency {
	namespace HardwareExtensionInterface {
		namespace Interop {
			/// <summary>Available pins.</summary>
			enum class Pins : uint8_t {
				IO1 = 0x0,
				IO2 = 0x1,
				IOA3 = 0x2,
				IOA4 = 0x3,
				IO5 = 0x4,
				IO6 = 0x5,
				IO7 = 0x6,
				IO8 = 0x7
			};
		} //namespace Interop
	} //namespace HardwareExtensionInterface
} //namespace Antilatency
namespace Antilatency {
	inline const char* enumToString(const Antilatency::HardwareExtensionInterface::Interop::Pins& x) {
		switch (x) {
			case Antilatency::HardwareExtensionInterface::Interop::Pins::IO1: return "IO1";
			case Antilatency::HardwareExtensionInterface::Interop::Pins::IO2: return "IO2";
			case Antilatency::HardwareExtensionInterface::Interop::Pins::IOA3: return "IOA3";
			case Antilatency::HardwareExtensionInterface::Interop::Pins::IOA4: return "IOA4";
			case Antilatency::HardwareExtensionInterface::Interop::Pins::IO5: return "IO5";
			case Antilatency::HardwareExtensionInterface::Interop::Pins::IO6: return "IO6";
			case Antilatency::HardwareExtensionInterface::Interop::Pins::IO7: return "IO7";
			case Antilatency::HardwareExtensionInterface::Interop::Pins::IO8: return "IO8";
		}
		return "";
	}
} //namespace Antilatency

namespace Antilatency {
	namespace HardwareExtensionInterface {
		namespace Interop {
			namespace Constants {
				constexpr uint32_t MaxInputPinsCount = 8U;
				constexpr uint32_t MaxOutputPinsCount = 8U;
				constexpr uint32_t MaxAnalogPinsCount = 2U;
				constexpr uint32_t MaxPulseCounterPinsCount = 2U;
				constexpr uint32_t MaxPwmPinsCount = 4U;
			} //namespace Constants
		} //namespace Interop
	} //namespace HardwareExtensionInterface
} //namespace Antilatency


#ifdef _MSC_VER
#pragma warning(pop)
#endif
