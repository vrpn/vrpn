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
#include <Antilatency.InterfaceContract.h>
#include <cstdint>
#include <exception>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4946)
#pragma warning(disable: 4265)
#endif
namespace Antilatency {
	namespace DeviceNetwork {
		namespace Interop {
			/// <summary>Task stream packet.</summary>
			struct Packet {
				/// <summary>Packet ID</summary>
				uint8_t id;
				/// <summary>Pointer to packet data.</summary>
				const void* data;
				/// <summary>Packet data size.</summary>
				uint32_t size;
			};
		} //namespace Interop
	} //namespace DeviceNetwork
} //namespace Antilatency

namespace Antilatency {
	namespace DeviceNetwork {
		namespace Interop {
			/* copy and paste this to implementer
			void packet(Antilatency::DeviceNetwork::Interop::Packet packet) {
				throw std::logic_error{"Method IDataReceiver.packet() is not implemented."};
			}
			*/
			/// <summary>Data receiver</summary>
			struct IDataReceiver : Antilatency::InterfaceContract::IInterface {
				struct VMT : Antilatency::InterfaceContract::IInterface::VMT {
					virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL packet(Antilatency::DeviceNetwork::Interop::Packet packet) = 0;
					static constexpr Antilatency::InterfaceContract::InterfaceID ID() {
						return Antilatency::InterfaceContract::InterfaceID{0xd86ef57e,0x603e,0x4d3d,{0x9f,0xfe,0xb1,0x45,0xab,0xd9,0xc0,0xaa}};
					}
				private:
					~VMT() = delete;
				};
				IDataReceiver() = default;
				IDataReceiver(std::nullptr_t) {}
				explicit IDataReceiver(VMT* pointer) : Antilatency::InterfaceContract::IInterface(pointer) {}
				template<typename T, typename = typename std::enable_if<std::is_base_of<IDataReceiver, T>::value>::type>
				IDataReceiver& operator = (const T& other) {
				    Antilatency::InterfaceContract::IInterface::operator=(other);
				    return *this;
				}
				template<class Implementer, class ... TArgs>
				static IDataReceiver create(TArgs&&... args) {
				    return *new Implementer(std::forward<TArgs>(args)...);
				}
				void attach(VMT* other) ANTILATENCY_NOEXCEPT {
				    Antilatency::InterfaceContract::IInterface::attach(other);
				}
				VMT* detach() ANTILATENCY_NOEXCEPT {
				    return reinterpret_cast<VMT*>(Antilatency::InterfaceContract::IInterface::detach());
				}
				/// <summary>Write packet to data receiver</summary>
				/// <param name = "packet">
				/// Packet with data
				/// </param>
				void packet(Antilatency::DeviceNetwork::Interop::Packet packet) {
					handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->packet(packet));
				}
			};
		} //namespace Interop
	} //namespace DeviceNetwork
} //namespace Antilatency
namespace Antilatency {
	namespace InterfaceContract {
		namespace Details {
			template<typename Implementer, typename LifeTimeController>
			struct InterfaceRemap<Antilatency::DeviceNetwork::Interop::IDataReceiver, Implementer, LifeTimeController> : Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController> {
			public:
			    InterfaceRemap() = default;
			    static bool isInterfaceSupported(const Antilatency::InterfaceContract::InterfaceID& id) {
					if (id == Antilatency::DeviceNetwork::Interop::IDataReceiver::VMT::ID()) {
						return true;
					}
					return Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController>::isInterfaceSupported(id);
				}
			public:
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL packet(Antilatency::DeviceNetwork::Interop::Packet packet) {
					try {
						this->_object->packet(packet);
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
			};
		} //namespace Details
	} //namespace InterfaceContract
} //namespace Antilatency

namespace Antilatency {
	namespace DeviceNetwork {
		namespace Interop {
			namespace Constants {
				static constexpr const char* FirmwareNameKey = "sys/FirmwareName";
				static constexpr const char* FirmwareVersionKey = "sys/FirmwareVersion";
				static constexpr const char* HardwareNameKey = "sys/HardwareName";
				static constexpr const char* HardwareVersionKey = "sys/HardwareVersion";
				static constexpr const char* HardwareSerialNumberKey = "sys/HardwareSerialNumber";
				static constexpr const char* ImageVersionKey = "sys/ImageVersion";
			} //namespace Constants
		} //namespace Interop
	} //namespace DeviceNetwork
} //namespace Antilatency

namespace Antilatency {
	namespace DeviceNetwork {
		namespace Interop {
			struct WriteBufferOverflowException : std::exception {
			};
		} //namespace Interop
	} //namespace DeviceNetwork
} //namespace Antilatency


#ifdef _MSC_VER
#pragma warning(pop)
#endif
