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
#include <Antilatency.DeviceNetwork.Interop.h>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4946)
#pragma warning(disable: 4265)
#endif
namespace Antilatency {
	namespace DeviceNetwork {
		enum class UsbVendorId : uint16_t {
			Antilatency = 0x3237,
			AntilatencyLegacy = 0x301
		};
	} //namespace DeviceNetwork
} //namespace Antilatency
namespace Antilatency {
	inline const char* enumToString(const Antilatency::DeviceNetwork::UsbVendorId& x) {
		switch (x) {
			case Antilatency::DeviceNetwork::UsbVendorId::Antilatency: return "Antilatency";
			case Antilatency::DeviceNetwork::UsbVendorId::AntilatencyLegacy: return "AntilatencyLegacy";
		}
		return "";
	}
} //namespace Antilatency

namespace Antilatency {
	namespace DeviceNetwork {
		/// <summary>USB device identifier.</summary>
		struct UsbDeviceType {
			/// <summary>USB device vendor ID. Default value for Antilatency devices is 0x3237</summary>
			Antilatency::DeviceNetwork::UsbVendorId vid;
			/// <summary>USB device product ID. Default value for Antilatency Sockets is 0x0000</summary>
			uint16_t pid;
		};
	} //namespace DeviceNetwork
} //namespace Antilatency

namespace Antilatency {
	namespace DeviceNetwork {
		/// <summary>The handle to the Antilatency Device Network device. Every time a device is connected, a unique handle will be applied to it. Therefore, when the device turns off, the NodeStatus for its node becomes Invalid. After reconnection, the devices receives a new NodeHandle.</summary>
		enum class NodeHandle : uint32_t {
			/// <summary>Any socket node connected directly by USB to PC, smartphone or HMD, will have Null as its parent.</summary>
			Null = 0x0
		};
		ANTILATENCY_ENUM_INTEGER_BEHAVIOUR_UNSIGNED(NodeHandle,uint32_t)
	} //namespace DeviceNetwork
} //namespace Antilatency
namespace Antilatency {
	inline const char* enumToString(const Antilatency::DeviceNetwork::NodeHandle& x) {
		switch (x) {
			case Antilatency::DeviceNetwork::NodeHandle::Null: return "Null";
		}
		return "";
	}
} //namespace Antilatency

namespace Antilatency {
	namespace DeviceNetwork {
		/// <summary>Defines different node conditions.</summary>
		enum class NodeStatus : int32_t {
			/// <summary>The node in connected and no task is currently running on it. Any supported task can be started on it.</summary>
			Idle = 0x0,
			/// <summary>The node in connected and a task is currently running on it. Before running any task on such node, you need to stop the current task first.</summary>
			TaskRunning = 0x1,
			/// <summary>The node in not valid, no tasks can be executed on it.</summary>
			Invalid = 0x2
		};
	} //namespace DeviceNetwork
} //namespace Antilatency
namespace Antilatency {
	inline const char* enumToString(const Antilatency::DeviceNetwork::NodeStatus& x) {
		switch (x) {
			case Antilatency::DeviceNetwork::NodeStatus::Idle: return "Idle";
			case Antilatency::DeviceNetwork::NodeStatus::TaskRunning: return "TaskRunning";
			case Antilatency::DeviceNetwork::NodeStatus::Invalid: return "Invalid";
		}
		return "";
	}
} //namespace Antilatency

namespace Antilatency {
	namespace DeviceNetwork {
		/* copy and paste this to implementer
		std::vector<Antilatency::DeviceNetwork::Interop::Packet> getPackets() {
			throw std::logic_error{"Method ISynchronousConnection.getPackets() is not implemented."};
		}
		std::vector<Antilatency::DeviceNetwork::Interop::Packet> getAvailablePackets(Antilatency::InterfaceContract::LongBool& taskFinished) {
			throw std::logic_error{"Method ISynchronousConnection.getAvailablePackets() is not implemented."};
		}
		Antilatency::InterfaceContract::LongBool writePacket(Antilatency::DeviceNetwork::Interop::Packet packet) {
			throw std::logic_error{"Method ISynchronousConnection.writePacket() is not implemented."};
		}
		*/
		/// <summary>Synchronous task read/write stream.</summary>
		struct ISynchronousConnection : Antilatency::InterfaceContract::IInterface {
			struct VMT : Antilatency::InterfaceContract::IInterface::VMT {
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getPackets(Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getAvailablePackets(Antilatency::InterfaceContract::LongBool& taskFinished, Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL writePacket(Antilatency::DeviceNetwork::Interop::Packet packet, Antilatency::InterfaceContract::LongBool& result) = 0;
				static constexpr Antilatency::InterfaceContract::InterfaceID ID() {
					return Antilatency::InterfaceContract::InterfaceID{0xaf7402e8,0x2a23,0x442b,{0x82,0x30,0xd4,0x1f,0x55,0xef,0x5d,0xc0}};
				}
			private:
				~VMT() = delete;
			};
			ISynchronousConnection() = default;
			ISynchronousConnection(std::nullptr_t) {}
			explicit ISynchronousConnection(VMT* pointer) : Antilatency::InterfaceContract::IInterface(pointer) {}
			template<typename T, typename = typename std::enable_if<std::is_base_of<ISynchronousConnection, T>::value>::type>
			ISynchronousConnection& operator = (const T& other) {
			    Antilatency::InterfaceContract::IInterface::operator=(other);
			    return *this;
			}
			template<class Implementer, class ... TArgs>
			static ISynchronousConnection create(TArgs&&... args) {
			    return *new Implementer(std::forward<TArgs>(args)...);
			}
			void attach(VMT* other) ANTILATENCY_NOEXCEPT {
			    Antilatency::InterfaceContract::IInterface::attach(other);
			}
			VMT* detach() ANTILATENCY_NOEXCEPT {
			    return reinterpret_cast<VMT*>(Antilatency::InterfaceContract::IInterface::detach());
			}
			/// <summary>Get received packets. Blocks until any packets received or task finished.</summary>
			/// <returns>Received packets array. Zero packets count if task is finished.</returns>
			std::vector<Antilatency::DeviceNetwork::Interop::Packet> getPackets() {
				std::vector<Antilatency::DeviceNetwork::Interop::Packet> result;
				auto resultMarshaler = Antilatency::InterfaceContract::Details::ArrayOutMarshaler::create(result);
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->getPackets(resultMarshaler));
				return result;
			}
			/// <summary>Get received packets.</summary>
			/// <param name = "taskFinished">
			/// Is task finished.
			/// </param>
			/// <returns>Received packets array. Zero packets count if no packets received.</returns>
			std::vector<Antilatency::DeviceNetwork::Interop::Packet> getAvailablePackets(Antilatency::InterfaceContract::LongBool& taskFinished) {
				std::vector<Antilatency::DeviceNetwork::Interop::Packet> result;
				auto resultMarshaler = Antilatency::InterfaceContract::Details::ArrayOutMarshaler::create(result);
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->getAvailablePackets(taskFinished, resultMarshaler));
				return result;
			}
			Antilatency::InterfaceContract::LongBool writePacket(Antilatency::DeviceNetwork::Interop::Packet packet) {
				Antilatency::InterfaceContract::LongBool result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->writePacket(packet, result));
				return result;
			}
		};
	} //namespace DeviceNetwork
} //namespace Antilatency
namespace Antilatency {
	namespace InterfaceContract {
		namespace Details {
			template<typename Implementer, typename LifeTimeController>
			struct InterfaceRemap<Antilatency::DeviceNetwork::ISynchronousConnection, Implementer, LifeTimeController> : Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController> {
			public:
			    InterfaceRemap() = default;
			    static bool isInterfaceSupported(const Antilatency::InterfaceContract::InterfaceID& id) {
					if (id == Antilatency::DeviceNetwork::ISynchronousConnection::VMT::ID()) {
						return true;
					}
					return Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController>::isInterfaceSupported(id);
				}
			public:
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getPackets(Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate result) {
					try {
						result = this->_object->getPackets();
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getAvailablePackets(Antilatency::InterfaceContract::LongBool& taskFinished, Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate result) {
					try {
						result = this->_object->getAvailablePackets(taskFinished);
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL writePacket(Antilatency::DeviceNetwork::Interop::Packet packet, Antilatency::InterfaceContract::LongBool& result) {
					try {
						result = this->_object->writePacket(packet);
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
		/* copy and paste this to implementer
		Antilatency::InterfaceContract::LongBool isTaskFinished() {
			throw std::logic_error{"Method ICotask.isTaskFinished() is not implemented."};
		}
		*/
		struct ICotask : Antilatency::InterfaceContract::IInterface {
			struct VMT : Antilatency::InterfaceContract::IInterface::VMT {
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL isTaskFinished(Antilatency::InterfaceContract::LongBool& result) = 0;
				static constexpr Antilatency::InterfaceContract::InterfaceID ID() {
					return Antilatency::InterfaceContract::InterfaceID{0xfd95f649,0x562a,0x4819,{0xa8,0x16,0x26,0xb7,0x6c,0xd9,0xd8,0xd6}};
				}
			private:
				~VMT() = delete;
			};
			ICotask() = default;
			ICotask(std::nullptr_t) {}
			explicit ICotask(VMT* pointer) : Antilatency::InterfaceContract::IInterface(pointer) {}
			template<typename T, typename = typename std::enable_if<std::is_base_of<ICotask, T>::value>::type>
			ICotask& operator = (const T& other) {
			    Antilatency::InterfaceContract::IInterface::operator=(other);
			    return *this;
			}
			template<class Implementer, class ... TArgs>
			static ICotask create(TArgs&&... args) {
			    return *new Implementer(std::forward<TArgs>(args)...);
			}
			void attach(VMT* other) ANTILATENCY_NOEXCEPT {
			    Antilatency::InterfaceContract::IInterface::attach(other);
			}
			VMT* detach() ANTILATENCY_NOEXCEPT {
			    return reinterpret_cast<VMT*>(Antilatency::InterfaceContract::IInterface::detach());
			}
			Antilatency::InterfaceContract::LongBool isTaskFinished() {
				Antilatency::InterfaceContract::LongBool result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->isTaskFinished(result));
				return result;
			}
		};
	} //namespace DeviceNetwork
} //namespace Antilatency
namespace Antilatency {
	namespace InterfaceContract {
		namespace Details {
			template<typename Implementer, typename LifeTimeController>
			struct InterfaceRemap<Antilatency::DeviceNetwork::ICotask, Implementer, LifeTimeController> : Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController> {
			public:
			    InterfaceRemap() = default;
			    static bool isInterfaceSupported(const Antilatency::InterfaceContract::InterfaceID& id) {
					if (id == Antilatency::DeviceNetwork::ICotask::VMT::ID()) {
						return true;
					}
					return Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController>::isInterfaceSupported(id);
				}
			public:
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL isTaskFinished(Antilatency::InterfaceContract::LongBool& result) {
					try {
						result = this->_object->isTaskFinished();
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
		/* copy and paste this to implementer
		std::string getPropertyKey(uint32_t index) {
			throw std::logic_error{"Method IPropertyCotask.getPropertyKey() is not implemented."};
		}
		std::string getStringProperty(const std::string& key) {
			throw std::logic_error{"Method IPropertyCotask.getStringProperty() is not implemented."};
		}
		void setStringProperty(const std::string& key, const std::string& value) {
			throw std::logic_error{"Method IPropertyCotask.setStringProperty() is not implemented."};
		}
		std::vector<uint8_t> getBinaryProperty(const std::string& key) {
			throw std::logic_error{"Method IPropertyCotask.getBinaryProperty() is not implemented."};
		}
		void setBinaryProperty(const std::string& key, const std::vector<uint8_t>& value) {
			throw std::logic_error{"Method IPropertyCotask.setBinaryProperty() is not implemented."};
		}
		void deleteProperty(const std::string& key) {
			throw std::logic_error{"Method IPropertyCotask.deleteProperty() is not implemented."};
		}
		*/
		struct IPropertyCotask : Antilatency::DeviceNetwork::ICotask {
			struct VMT : Antilatency::DeviceNetwork::ICotask::VMT {
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getPropertyKey(uint32_t index, Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getStringProperty(Antilatency::InterfaceContract::Details::ArrayInMarshaler::Intermediate key, Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL setStringProperty(Antilatency::InterfaceContract::Details::ArrayInMarshaler::Intermediate key, Antilatency::InterfaceContract::Details::ArrayInMarshaler::Intermediate value) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getBinaryProperty(Antilatency::InterfaceContract::Details::ArrayInMarshaler::Intermediate key, Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL setBinaryProperty(Antilatency::InterfaceContract::Details::ArrayInMarshaler::Intermediate key, Antilatency::InterfaceContract::Details::ArrayInMarshaler::Intermediate value) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL deleteProperty(Antilatency::InterfaceContract::Details::ArrayInMarshaler::Intermediate key) = 0;
				static constexpr Antilatency::InterfaceContract::InterfaceID ID() {
					return Antilatency::InterfaceContract::InterfaceID{0x81ea9312,0xf66e,0x4708,{0xac,0xd1,0xd4,0x0a,0x3e,0x6e,0xf9,0xaa}};
				}
			private:
				~VMT() = delete;
			};
			IPropertyCotask() = default;
			IPropertyCotask(std::nullptr_t) {}
			explicit IPropertyCotask(VMT* pointer) : Antilatency::DeviceNetwork::ICotask(pointer) {}
			template<typename T, typename = typename std::enable_if<std::is_base_of<IPropertyCotask, T>::value>::type>
			IPropertyCotask& operator = (const T& other) {
			    Antilatency::DeviceNetwork::ICotask::operator=(other);
			    return *this;
			}
			template<class Implementer, class ... TArgs>
			static IPropertyCotask create(TArgs&&... args) {
			    return *new Implementer(std::forward<TArgs>(args)...);
			}
			void attach(VMT* other) ANTILATENCY_NOEXCEPT {
			    Antilatency::DeviceNetwork::ICotask::attach(other);
			}
			VMT* detach() ANTILATENCY_NOEXCEPT {
			    return reinterpret_cast<VMT*>(Antilatency::DeviceNetwork::ICotask::detach());
			}
			std::string getPropertyKey(uint32_t index) {
				std::string result;
				auto resultMarshaler = Antilatency::InterfaceContract::Details::ArrayOutMarshaler::create(result);
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->getPropertyKey(index, resultMarshaler));
				return result;
			}
			std::string getStringProperty(const std::string& key) {
				std::string result;
				auto keyMarshaler = Antilatency::InterfaceContract::Details::ArrayInMarshaler::create(key);
				auto resultMarshaler = Antilatency::InterfaceContract::Details::ArrayOutMarshaler::create(result);
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->getStringProperty(keyMarshaler, resultMarshaler));
				return result;
			}
			void setStringProperty(const std::string& key, const std::string& value) {
				auto keyMarshaler = Antilatency::InterfaceContract::Details::ArrayInMarshaler::create(key);
				auto valueMarshaler = Antilatency::InterfaceContract::Details::ArrayInMarshaler::create(value);
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->setStringProperty(keyMarshaler, valueMarshaler));
			}
			std::vector<uint8_t> getBinaryProperty(const std::string& key) {
				std::vector<uint8_t> result;
				auto keyMarshaler = Antilatency::InterfaceContract::Details::ArrayInMarshaler::create(key);
				auto resultMarshaler = Antilatency::InterfaceContract::Details::ArrayOutMarshaler::create(result);
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->getBinaryProperty(keyMarshaler, resultMarshaler));
				return result;
			}
			void setBinaryProperty(const std::string& key, const std::vector<uint8_t>& value) {
				auto keyMarshaler = Antilatency::InterfaceContract::Details::ArrayInMarshaler::create(key);
				auto valueMarshaler = Antilatency::InterfaceContract::Details::ArrayInMarshaler::create(value);
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->setBinaryProperty(keyMarshaler, valueMarshaler));
			}
			void deleteProperty(const std::string& key) {
				auto keyMarshaler = Antilatency::InterfaceContract::Details::ArrayInMarshaler::create(key);
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->deleteProperty(keyMarshaler));
			}
		};
	} //namespace DeviceNetwork
} //namespace Antilatency
namespace Antilatency {
	namespace InterfaceContract {
		namespace Details {
			template<typename Implementer, typename LifeTimeController>
			struct InterfaceRemap<Antilatency::DeviceNetwork::IPropertyCotask, Implementer, LifeTimeController> : Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::DeviceNetwork::ICotask, Implementer, LifeTimeController> {
			public:
			    InterfaceRemap() = default;
			    static bool isInterfaceSupported(const Antilatency::InterfaceContract::InterfaceID& id) {
					if (id == Antilatency::DeviceNetwork::IPropertyCotask::VMT::ID()) {
						return true;
					}
					return Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::DeviceNetwork::ICotask, Implementer, LifeTimeController>::isInterfaceSupported(id);
				}
			public:
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getPropertyKey(uint32_t index, Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate result) {
					try {
						result = this->_object->getPropertyKey(index);
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getStringProperty(Antilatency::InterfaceContract::Details::ArrayInMarshaler::Intermediate key, Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate result) {
					try {
						result = this->_object->getStringProperty(key);
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL setStringProperty(Antilatency::InterfaceContract::Details::ArrayInMarshaler::Intermediate key, Antilatency::InterfaceContract::Details::ArrayInMarshaler::Intermediate value) {
					try {
						this->_object->setStringProperty(key, value);
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getBinaryProperty(Antilatency::InterfaceContract::Details::ArrayInMarshaler::Intermediate key, Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate result) {
					try {
						result = this->_object->getBinaryProperty(key);
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL setBinaryProperty(Antilatency::InterfaceContract::Details::ArrayInMarshaler::Intermediate key, Antilatency::InterfaceContract::Details::ArrayInMarshaler::Intermediate value) {
					try {
						this->_object->setBinaryProperty(key, value);
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL deleteProperty(Antilatency::InterfaceContract::Details::ArrayInMarshaler::Intermediate key) {
					try {
						this->_object->deleteProperty(key);
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
		/* copy and paste this to implementer
		uint32_t getUpdateId() {
			throw std::logic_error{"Method INetwork.getUpdateId() is not implemented."};
		}
		std::vector<Antilatency::DeviceNetwork::UsbDeviceType> getDeviceTypes() {
			throw std::logic_error{"Method INetwork.getDeviceTypes() is not implemented."};
		}
		std::vector<Antilatency::DeviceNetwork::NodeHandle> getNodes() {
			throw std::logic_error{"Method INetwork.getNodes() is not implemented."};
		}
		Antilatency::DeviceNetwork::NodeStatus nodeGetStatus(Antilatency::DeviceNetwork::NodeHandle node) {
			throw std::logic_error{"Method INetwork.nodeGetStatus() is not implemented."};
		}
		Antilatency::InterfaceContract::LongBool nodeIsTaskSupported(Antilatency::DeviceNetwork::NodeHandle node, Antilatency::InterfaceContract::InterfaceID taskId) {
			throw std::logic_error{"Method INetwork.nodeIsTaskSupported() is not implemented."};
		}
		Antilatency::DeviceNetwork::NodeHandle nodeGetParent(Antilatency::DeviceNetwork::NodeHandle node) {
			throw std::logic_error{"Method INetwork.nodeGetParent() is not implemented."};
		}
		std::string nodeGetPhysicalPath(Antilatency::DeviceNetwork::NodeHandle node) {
			throw std::logic_error{"Method INetwork.nodeGetPhysicalPath() is not implemented."};
		}
		Antilatency::DeviceNetwork::Interop::IDataReceiver nodeStartTask(Antilatency::DeviceNetwork::NodeHandle node, Antilatency::InterfaceContract::InterfaceID taskId, Antilatency::DeviceNetwork::Interop::IDataReceiver taskDataReceiver) {
			throw std::logic_error{"Method INetwork.nodeStartTask() is not implemented."};
		}
		Antilatency::DeviceNetwork::ISynchronousConnection nodeStartTask2(Antilatency::DeviceNetwork::NodeHandle node, Antilatency::InterfaceContract::InterfaceID taskId) {
			throw std::logic_error{"Method INetwork.nodeStartTask2() is not implemented."};
		}
		std::string nodeGetStringProperty(Antilatency::DeviceNetwork::NodeHandle node, const std::string& key) {
			throw std::logic_error{"Method INetwork.nodeGetStringProperty() is not implemented."};
		}
		std::vector<uint8_t> nodeGetBinaryProperty(Antilatency::DeviceNetwork::NodeHandle node, const std::string& key) {
			throw std::logic_error{"Method INetwork.nodeGetBinaryProperty() is not implemented."};
		}
		Antilatency::DeviceNetwork::IPropertyCotask nodeStartPropertyTask(Antilatency::DeviceNetwork::NodeHandle node) {
			throw std::logic_error{"Method INetwork.nodeStartPropertyTask() is not implemented."};
		}
		*/
		/// <summary>Network of Nodes</summary>
		struct INetwork : Antilatency::InterfaceContract::IInterface {
			struct VMT : Antilatency::InterfaceContract::IInterface::VMT {
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getUpdateId(uint32_t& result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getDeviceTypes(Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getNodes(Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL nodeGetStatus(Antilatency::DeviceNetwork::NodeHandle node, Antilatency::DeviceNetwork::NodeStatus& result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL nodeIsTaskSupported(Antilatency::DeviceNetwork::NodeHandle node, Antilatency::InterfaceContract::InterfaceID taskId, Antilatency::InterfaceContract::LongBool& result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL nodeGetParent(Antilatency::DeviceNetwork::NodeHandle node, Antilatency::DeviceNetwork::NodeHandle& result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL nodeGetPhysicalPath(Antilatency::DeviceNetwork::NodeHandle node, Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL nodeStartTask(Antilatency::DeviceNetwork::NodeHandle node, Antilatency::InterfaceContract::InterfaceID taskId, Antilatency::DeviceNetwork::Interop::IDataReceiver::VMT* taskDataReceiver, Antilatency::DeviceNetwork::Interop::IDataReceiver& result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL nodeStartTask2(Antilatency::DeviceNetwork::NodeHandle node, Antilatency::InterfaceContract::InterfaceID taskId, Antilatency::DeviceNetwork::ISynchronousConnection& result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL nodeGetStringProperty(Antilatency::DeviceNetwork::NodeHandle node, Antilatency::InterfaceContract::Details::ArrayInMarshaler::Intermediate key, Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL nodeGetBinaryProperty(Antilatency::DeviceNetwork::NodeHandle node, Antilatency::InterfaceContract::Details::ArrayInMarshaler::Intermediate key, Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL nodeStartPropertyTask(Antilatency::DeviceNetwork::NodeHandle node, Antilatency::DeviceNetwork::IPropertyCotask& result) = 0;
				static constexpr Antilatency::InterfaceContract::InterfaceID ID() {
					return Antilatency::InterfaceContract::InterfaceID{0x4cb2369c,0x7a66,0x4e85,{0x9a,0x0c,0xdb,0xc8,0x9f,0xc1,0xc7,0x5e}};
				}
			private:
				~VMT() = delete;
			};
			INetwork() = default;
			INetwork(std::nullptr_t) {}
			explicit INetwork(VMT* pointer) : Antilatency::InterfaceContract::IInterface(pointer) {}
			template<typename T, typename = typename std::enable_if<std::is_base_of<INetwork, T>::value>::type>
			INetwork& operator = (const T& other) {
			    Antilatency::InterfaceContract::IInterface::operator=(other);
			    return *this;
			}
			template<class Implementer, class ... TArgs>
			static INetwork create(TArgs&&... args) {
			    return *new Implementer(std::forward<TArgs>(args)...);
			}
			void attach(VMT* other) ANTILATENCY_NOEXCEPT {
			    Antilatency::InterfaceContract::IInterface::attach(other);
			}
			VMT* detach() ANTILATENCY_NOEXCEPT {
			    return reinterpret_cast<VMT*>(Antilatency::InterfaceContract::IInterface::detach());
			}
			/// <summary>Every time any supported device is connected or disconnected, the update ID will be incremented. You can use this method to check if there are any changes in the ADN.</summary>
			/// <returns>Current factory update ID.</returns>
			uint32_t getUpdateId() {
				uint32_t result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->getUpdateId(result));
				return result;
			}
			/// <summary>Get the USB device types selected to work with this factory instance.</summary>
			/// <returns>The array of the USB device identifiers which this factory instance is working with.</returns>
			std::vector<Antilatency::DeviceNetwork::UsbDeviceType> getDeviceTypes() {
				std::vector<Antilatency::DeviceNetwork::UsbDeviceType> result;
				auto resultMarshaler = Antilatency::InterfaceContract::Details::ArrayOutMarshaler::create(result);
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->getDeviceTypes(resultMarshaler));
				return result;
			}
			/// <summary>Get all currently connected nodes.</summary>
			std::vector<Antilatency::DeviceNetwork::NodeHandle> getNodes() {
				std::vector<Antilatency::DeviceNetwork::NodeHandle> result;
				auto resultMarshaler = Antilatency::InterfaceContract::Details::ArrayOutMarshaler::create(result);
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->getNodes(resultMarshaler));
				return result;
			}
			/// <summary>Get the current NodeStatus for the node.</summary>
			/// <param name = "node">
			/// Node handle to get status of.
			/// </param>
			Antilatency::DeviceNetwork::NodeStatus nodeGetStatus(Antilatency::DeviceNetwork::NodeHandle node) {
				Antilatency::DeviceNetwork::NodeStatus result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->nodeGetStatus(node, result));
				return result;
			}
			/// <summary>Checks if the task is supported by the node.</summary>
			/// <param name = "node">
			/// Node handle.
			/// </param>
			/// <param name = "taskId">
			/// Task ID.
			/// </param>
			/// <returns>True if node supports such task, otherwise false.</returns>
			Antilatency::InterfaceContract::LongBool nodeIsTaskSupported(Antilatency::DeviceNetwork::NodeHandle node, Antilatency::InterfaceContract::InterfaceID taskId) {
				Antilatency::InterfaceContract::LongBool result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->nodeIsTaskSupported(node, taskId, result));
				return result;
			}
			/// <summary>Get parent for the node, in case of USB node this method will return Antilatency.DeviceNetwork.NodeHandle.Null</summary>
			/// <param name = "node">
			/// Node handle.
			/// </param>
			Antilatency::DeviceNetwork::NodeHandle nodeGetParent(Antilatency::DeviceNetwork::NodeHandle node) {
				Antilatency::DeviceNetwork::NodeHandle result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->nodeGetParent(node, result));
				return result;
			}
			/// <summary>Physical address path to the first level device that contains this node in the network tree.</summary>
			/// <param name = "node">
			/// Node handle.
			/// </param>
			/// <returns>String represents physical path to first level device (connected via USB).</returns>
			std::string nodeGetPhysicalPath(Antilatency::DeviceNetwork::NodeHandle node) {
				std::string result;
				auto resultMarshaler = Antilatency::InterfaceContract::Details::ArrayOutMarshaler::create(result);
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->nodeGetPhysicalPath(node, resultMarshaler));
				return result;
			}
			/// <summary>Run the task on the node with the asynchronous packet receive API.</summary>
			/// <param name = "node">
			/// Node handle to start task on.
			/// </param>
			/// <param name = "taskId">
			/// Task ID.
			/// </param>
			/// <param name = "taskDataReceiver">
			/// Task data receiver.
			/// </param>
			Antilatency::DeviceNetwork::Interop::IDataReceiver nodeStartTask(Antilatency::DeviceNetwork::NodeHandle node, Antilatency::InterfaceContract::InterfaceID taskId, Antilatency::DeviceNetwork::Interop::IDataReceiver taskDataReceiver) {
				Antilatency::DeviceNetwork::Interop::IDataReceiver result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->nodeStartTask(node, taskId, taskDataReceiver.detach(), result));
				return result;
			}
			/// <summary>Run the task on the node with the synchronous packet receive API.</summary>
			/// <param name = "node">
			/// Node handle to start task on.
			/// </param>
			/// <param name = "taskId">
			/// Task ID.
			/// </param>
			Antilatency::DeviceNetwork::ISynchronousConnection nodeStartTask2(Antilatency::DeviceNetwork::NodeHandle node, Antilatency::InterfaceContract::InterfaceID taskId) {
				Antilatency::DeviceNetwork::ISynchronousConnection result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->nodeStartTask2(node, taskId, result));
				return result;
			}
			/// <summary>Get the node's string property value.</summary>
			/// <param name = "node">
			/// Node handle to get string property from.
			/// </param>
			/// <param name = "key">
			/// Property key. List of predefined properties you can find in the documentation, also there are some properties defined in the Antilatency.DeviceNetwork.Constants that is valid for every Antilatency device.
			/// </param>
			/// <returns>The node's string property value.</returns>
			std::string nodeGetStringProperty(Antilatency::DeviceNetwork::NodeHandle node, const std::string& key) {
				std::string result;
				auto keyMarshaler = Antilatency::InterfaceContract::Details::ArrayInMarshaler::create(key);
				auto resultMarshaler = Antilatency::InterfaceContract::Details::ArrayOutMarshaler::create(result);
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->nodeGetStringProperty(node, keyMarshaler, resultMarshaler));
				return result;
			}
			/// <summary>Get the node's binary property value.</summary>
			/// <param name = "node">
			/// Node handle to get binary property from.
			/// </param>
			/// <param name = "key">
			/// Property key.
			/// </param>
			/// <returns>The node's binary property value.</returns>
			std::vector<uint8_t> nodeGetBinaryProperty(Antilatency::DeviceNetwork::NodeHandle node, const std::string& key) {
				std::vector<uint8_t> result;
				auto keyMarshaler = Antilatency::InterfaceContract::Details::ArrayInMarshaler::create(key);
				auto resultMarshaler = Antilatency::InterfaceContract::Details::ArrayOutMarshaler::create(result);
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->nodeGetBinaryProperty(node, keyMarshaler, resultMarshaler));
				return result;
			}
			/// <summary>Start the property task on the node.</summary>
			/// <param name = "node">
			/// Node handle to start property task on.
			/// </param>
			Antilatency::DeviceNetwork::IPropertyCotask nodeStartPropertyTask(Antilatency::DeviceNetwork::NodeHandle node) {
				Antilatency::DeviceNetwork::IPropertyCotask result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->nodeStartPropertyTask(node, result));
				return result;
			}
		};
	} //namespace DeviceNetwork
} //namespace Antilatency
namespace Antilatency {
	namespace InterfaceContract {
		namespace Details {
			template<typename Implementer, typename LifeTimeController>
			struct InterfaceRemap<Antilatency::DeviceNetwork::INetwork, Implementer, LifeTimeController> : Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController> {
			public:
			    InterfaceRemap() = default;
			    static bool isInterfaceSupported(const Antilatency::InterfaceContract::InterfaceID& id) {
					if (id == Antilatency::DeviceNetwork::INetwork::VMT::ID()) {
						return true;
					}
					return Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController>::isInterfaceSupported(id);
				}
			public:
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getUpdateId(uint32_t& result) {
					try {
						result = this->_object->getUpdateId();
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getDeviceTypes(Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate result) {
					try {
						result = this->_object->getDeviceTypes();
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getNodes(Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate result) {
					try {
						result = this->_object->getNodes();
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL nodeGetStatus(Antilatency::DeviceNetwork::NodeHandle node, Antilatency::DeviceNetwork::NodeStatus& result) {
					try {
						result = this->_object->nodeGetStatus(node);
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL nodeIsTaskSupported(Antilatency::DeviceNetwork::NodeHandle node, Antilatency::InterfaceContract::InterfaceID taskId, Antilatency::InterfaceContract::LongBool& result) {
					try {
						result = this->_object->nodeIsTaskSupported(node, taskId);
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL nodeGetParent(Antilatency::DeviceNetwork::NodeHandle node, Antilatency::DeviceNetwork::NodeHandle& result) {
					try {
						result = this->_object->nodeGetParent(node);
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL nodeGetPhysicalPath(Antilatency::DeviceNetwork::NodeHandle node, Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate result) {
					try {
						result = this->_object->nodeGetPhysicalPath(node);
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL nodeStartTask(Antilatency::DeviceNetwork::NodeHandle node, Antilatency::InterfaceContract::InterfaceID taskId, Antilatency::DeviceNetwork::Interop::IDataReceiver::VMT* taskDataReceiver, Antilatency::DeviceNetwork::Interop::IDataReceiver& result) {
					try {
						Antilatency::DeviceNetwork::Interop::IDataReceiver taskDataReceiverMarshaler;
						taskDataReceiverMarshaler.attach(taskDataReceiver);
						result = this->_object->nodeStartTask(node, taskId, taskDataReceiverMarshaler);
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL nodeStartTask2(Antilatency::DeviceNetwork::NodeHandle node, Antilatency::InterfaceContract::InterfaceID taskId, Antilatency::DeviceNetwork::ISynchronousConnection& result) {
					try {
						result = this->_object->nodeStartTask2(node, taskId);
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL nodeGetStringProperty(Antilatency::DeviceNetwork::NodeHandle node, Antilatency::InterfaceContract::Details::ArrayInMarshaler::Intermediate key, Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate result) {
					try {
						result = this->_object->nodeGetStringProperty(node, key);
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL nodeGetBinaryProperty(Antilatency::DeviceNetwork::NodeHandle node, Antilatency::InterfaceContract::Details::ArrayInMarshaler::Intermediate key, Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate result) {
					try {
						result = this->_object->nodeGetBinaryProperty(node, key);
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL nodeStartPropertyTask(Antilatency::DeviceNetwork::NodeHandle node, Antilatency::DeviceNetwork::IPropertyCotask& result) {
					try {
						result = this->_object->nodeStartPropertyTask(node);
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
		/// <summary>Antilatency Device Network log verbosity level.</summary>
		enum class LogLevel : int32_t {
			Trace = 0x0,
			Debug = 0x1,
			Info = 0x2,
			Warning = 0x3,
			Error = 0x4,
			Critical = 0x5,
			Off = 0x6
		};
	} //namespace DeviceNetwork
} //namespace Antilatency
namespace Antilatency {
	inline const char* enumToString(const Antilatency::DeviceNetwork::LogLevel& x) {
		switch (x) {
			case Antilatency::DeviceNetwork::LogLevel::Trace: return "Trace";
			case Antilatency::DeviceNetwork::LogLevel::Debug: return "Debug";
			case Antilatency::DeviceNetwork::LogLevel::Info: return "Info";
			case Antilatency::DeviceNetwork::LogLevel::Warning: return "Warning";
			case Antilatency::DeviceNetwork::LogLevel::Error: return "Error";
			case Antilatency::DeviceNetwork::LogLevel::Critical: return "Critical";
			case Antilatency::DeviceNetwork::LogLevel::Off: return "Off";
		}
		return "";
	}
} //namespace Antilatency

namespace Antilatency {
	namespace DeviceNetwork {
		/* copy and paste this to implementer
		Antilatency::DeviceNetwork::INetwork createNetwork(const std::vector<Antilatency::DeviceNetwork::UsbDeviceType>& deviceTypes) {
			throw std::logic_error{"Method ILibrary.createNetwork() is not implemented."};
		}
		std::string getVersion() {
			throw std::logic_error{"Method ILibrary.getVersion() is not implemented."};
		}
		void setLogLevel(Antilatency::DeviceNetwork::LogLevel level) {
			throw std::logic_error{"Method ILibrary.setLogLevel() is not implemented."};
		}
		*/
		/// <summary>Antilatency Device Network library entry point.</summary>
		struct ILibrary : Antilatency::InterfaceContract::IInterface {
			struct VMT : Antilatency::InterfaceContract::IInterface::VMT {
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL createNetwork(Antilatency::InterfaceContract::Details::ArrayInMarshaler::Intermediate deviceTypes, Antilatency::DeviceNetwork::INetwork& result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getVersion(Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL setLogLevel(Antilatency::DeviceNetwork::LogLevel level) = 0;
				static constexpr Antilatency::InterfaceContract::InterfaceID ID() {
					return Antilatency::InterfaceContract::InterfaceID{0x36219fe8,0x3ad9,0x4b70,{0x8c,0x47,0xa0,0x32,0xfe,0x0b,0x5c,0x10}};
				}
			private:
				~VMT() = delete;
			};
			ILibrary() = default;
			ILibrary(std::nullptr_t) {}
			explicit ILibrary(VMT* pointer) : Antilatency::InterfaceContract::IInterface(pointer) {}
			template<typename T, typename = typename std::enable_if<std::is_base_of<ILibrary, T>::value>::type>
			ILibrary& operator = (const T& other) {
			    Antilatency::InterfaceContract::IInterface::operator=(other);
			    return *this;
			}
			template<class Implementer, class ... TArgs>
			static ILibrary create(TArgs&&... args) {
			    return *new Implementer(std::forward<TArgs>(args)...);
			}
			void attach(VMT* other) ANTILATENCY_NOEXCEPT {
			    Antilatency::InterfaceContract::IInterface::attach(other);
			}
			VMT* detach() ANTILATENCY_NOEXCEPT {
			    return reinterpret_cast<VMT*>(Antilatency::InterfaceContract::IInterface::detach());
			}
			/// <summary>Create Antilatency Device Network object</summary>
			/// <param name = "deviceTypes">
			/// The array of USB device identifiers which will be used by network.
			/// </param>
			Antilatency::DeviceNetwork::INetwork createNetwork(const std::vector<Antilatency::DeviceNetwork::UsbDeviceType>& deviceTypes) {
				Antilatency::DeviceNetwork::INetwork result;
				auto deviceTypesMarshaler = Antilatency::InterfaceContract::Details::ArrayInMarshaler::create(deviceTypes);
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->createNetwork(deviceTypesMarshaler, result));
				return result;
			}
			/// <summary>Get the Antilatency Device Network library version.</summary>
			std::string getVersion() {
				std::string result;
				auto resultMarshaler = Antilatency::InterfaceContract::Details::ArrayOutMarshaler::create(result);
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->getVersion(resultMarshaler));
				return result;
			}
			/// <summary>Set the Antilatency Device Network log verbosity level.</summary>
			void setLogLevel(Antilatency::DeviceNetwork::LogLevel level) {
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->setLogLevel(level));
			}
		};
	} //namespace DeviceNetwork
} //namespace Antilatency
namespace Antilatency {
	namespace InterfaceContract {
		namespace Details {
			template<typename Implementer, typename LifeTimeController>
			struct InterfaceRemap<Antilatency::DeviceNetwork::ILibrary, Implementer, LifeTimeController> : Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController> {
			public:
			    InterfaceRemap() = default;
			    static bool isInterfaceSupported(const Antilatency::InterfaceContract::InterfaceID& id) {
					if (id == Antilatency::DeviceNetwork::ILibrary::VMT::ID()) {
						return true;
					}
					return Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController>::isInterfaceSupported(id);
				}
			public:
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL createNetwork(Antilatency::InterfaceContract::Details::ArrayInMarshaler::Intermediate deviceTypes, Antilatency::DeviceNetwork::INetwork& result) {
					try {
						result = this->_object->createNetwork(deviceTypes);
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getVersion(Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate result) {
					try {
						result = this->_object->getVersion();
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL setLogLevel(Antilatency::DeviceNetwork::LogLevel level) {
					try {
						this->_object->setLogLevel(level);
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
		/* copy and paste this to implementer
		Antilatency::InterfaceContract::LongBool isSupported(Antilatency::DeviceNetwork::INetwork network, Antilatency::DeviceNetwork::NodeHandle node) {
			throw std::logic_error{"Method ICotaskConstructor.isSupported() is not implemented."};
		}
		std::vector<Antilatency::DeviceNetwork::NodeHandle> findSupportedNodes(Antilatency::DeviceNetwork::INetwork network) {
			throw std::logic_error{"Method ICotaskConstructor.findSupportedNodes() is not implemented."};
		}
		*/
		struct ICotaskConstructor : Antilatency::InterfaceContract::IInterface {
			struct VMT : Antilatency::InterfaceContract::IInterface::VMT {
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL isSupported(Antilatency::DeviceNetwork::INetwork::VMT* network, Antilatency::DeviceNetwork::NodeHandle node, Antilatency::InterfaceContract::LongBool& result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL findSupportedNodes(Antilatency::DeviceNetwork::INetwork::VMT* network, Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate result) = 0;
				static constexpr Antilatency::InterfaceContract::InterfaceID ID() {
					return Antilatency::InterfaceContract::InterfaceID{0x9ecfa909,0xd13c,0x4c29,{0xa8,0x7e,0x89,0x25,0xb7,0x84,0x66,0x38}};
				}
			private:
				~VMT() = delete;
			};
			ICotaskConstructor() = default;
			ICotaskConstructor(std::nullptr_t) {}
			explicit ICotaskConstructor(VMT* pointer) : Antilatency::InterfaceContract::IInterface(pointer) {}
			template<typename T, typename = typename std::enable_if<std::is_base_of<ICotaskConstructor, T>::value>::type>
			ICotaskConstructor& operator = (const T& other) {
			    Antilatency::InterfaceContract::IInterface::operator=(other);
			    return *this;
			}
			template<class Implementer, class ... TArgs>
			static ICotaskConstructor create(TArgs&&... args) {
			    return *new Implementer(std::forward<TArgs>(args)...);
			}
			void attach(VMT* other) ANTILATENCY_NOEXCEPT {
			    Antilatency::InterfaceContract::IInterface::attach(other);
			}
			VMT* detach() ANTILATENCY_NOEXCEPT {
			    return reinterpret_cast<VMT*>(Antilatency::InterfaceContract::IInterface::detach());
			}
			Antilatency::InterfaceContract::LongBool isSupported(Antilatency::DeviceNetwork::INetwork network, Antilatency::DeviceNetwork::NodeHandle node) {
				Antilatency::InterfaceContract::LongBool result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->isSupported(network.detach(), node, result));
				return result;
			}
			std::vector<Antilatency::DeviceNetwork::NodeHandle> findSupportedNodes(Antilatency::DeviceNetwork::INetwork network) {
				std::vector<Antilatency::DeviceNetwork::NodeHandle> result;
				auto resultMarshaler = Antilatency::InterfaceContract::Details::ArrayOutMarshaler::create(result);
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->findSupportedNodes(network.detach(), resultMarshaler));
				return result;
			}
		};
	} //namespace DeviceNetwork
} //namespace Antilatency
namespace Antilatency {
	namespace InterfaceContract {
		namespace Details {
			template<typename Implementer, typename LifeTimeController>
			struct InterfaceRemap<Antilatency::DeviceNetwork::ICotaskConstructor, Implementer, LifeTimeController> : Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController> {
			public:
			    InterfaceRemap() = default;
			    static bool isInterfaceSupported(const Antilatency::InterfaceContract::InterfaceID& id) {
					if (id == Antilatency::DeviceNetwork::ICotaskConstructor::VMT::ID()) {
						return true;
					}
					return Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController>::isInterfaceSupported(id);
				}
			public:
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL isSupported(Antilatency::DeviceNetwork::INetwork::VMT* network, Antilatency::DeviceNetwork::NodeHandle node, Antilatency::InterfaceContract::LongBool& result) {
					try {
						Antilatency::DeviceNetwork::INetwork networkMarshaler;
						networkMarshaler.attach(network);
						result = this->_object->isSupported(networkMarshaler, node);
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL findSupportedNodes(Antilatency::DeviceNetwork::INetwork::VMT* network, Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate result) {
					try {
						Antilatency::DeviceNetwork::INetwork networkMarshaler;
						networkMarshaler.attach(network);
						result = this->_object->findSupportedNodes(networkMarshaler);
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
		/* copy and paste this to implementer
		float getBatteryLevel() {
			throw std::logic_error{"Method ICotaskBatteryPowered.getBatteryLevel() is not implemented."};
		}
		*/
		struct ICotaskBatteryPowered : Antilatency::DeviceNetwork::ICotask {
			struct VMT : Antilatency::DeviceNetwork::ICotask::VMT {
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getBatteryLevel(float& result) = 0;
				static constexpr Antilatency::InterfaceContract::InterfaceID ID() {
					return Antilatency::InterfaceContract::InterfaceID{0x1f3f7579,0x813e,0x4528,{0x82,0xf9,0x5a,0x5f,0xc3,0x5a,0x92,0x95}};
				}
			private:
				~VMT() = delete;
			};
			ICotaskBatteryPowered() = default;
			ICotaskBatteryPowered(std::nullptr_t) {}
			explicit ICotaskBatteryPowered(VMT* pointer) : Antilatency::DeviceNetwork::ICotask(pointer) {}
			template<typename T, typename = typename std::enable_if<std::is_base_of<ICotaskBatteryPowered, T>::value>::type>
			ICotaskBatteryPowered& operator = (const T& other) {
			    Antilatency::DeviceNetwork::ICotask::operator=(other);
			    return *this;
			}
			template<class Implementer, class ... TArgs>
			static ICotaskBatteryPowered create(TArgs&&... args) {
			    return *new Implementer(std::forward<TArgs>(args)...);
			}
			void attach(VMT* other) ANTILATENCY_NOEXCEPT {
			    Antilatency::DeviceNetwork::ICotask::attach(other);
			}
			VMT* detach() ANTILATENCY_NOEXCEPT {
			    return reinterpret_cast<VMT*>(Antilatency::DeviceNetwork::ICotask::detach());
			}
			/// <summary>Get actual battery level.</summary>
			/// <returns>Battery level in range 0 .. 1. Value 0 - empty battery, value 1 - full battery.</returns>
			float getBatteryLevel() {
				float result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->getBatteryLevel(result));
				return result;
			}
		};
	} //namespace DeviceNetwork
} //namespace Antilatency
namespace Antilatency {
	namespace InterfaceContract {
		namespace Details {
			template<typename Implementer, typename LifeTimeController>
			struct InterfaceRemap<Antilatency::DeviceNetwork::ICotaskBatteryPowered, Implementer, LifeTimeController> : Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::DeviceNetwork::ICotask, Implementer, LifeTimeController> {
			public:
			    InterfaceRemap() = default;
			    static bool isInterfaceSupported(const Antilatency::InterfaceContract::InterfaceID& id) {
					if (id == Antilatency::DeviceNetwork::ICotaskBatteryPowered::VMT::ID()) {
						return true;
					}
					return Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::DeviceNetwork::ICotask, Implementer, LifeTimeController>::isInterfaceSupported(id);
				}
			public:
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getBatteryLevel(float& result) {
					try {
						result = this->_object->getBatteryLevel();
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


#ifdef _MSC_VER
#pragma warning(pop)
#endif
