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
#include <Antilatency.DeviceNetwork.h>
#include <Antilatency.HardwareExtensionInterface.Interop.h>
#include <cstdint>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4946)
#pragma warning(disable: 4265)
#endif
namespace Antilatency {
	namespace HardwareExtensionInterface {
		/* copy and paste this to implementer
		Antilatency::HardwareExtensionInterface::Interop::PinState getState() {
			throw std::logic_error{"Method IInputPin.getState() is not implemented."};
		}
		*/
		/// <summary>Interface for reading state of pin in input mode.</summary>
		struct IInputPin : Antilatency::InterfaceContract::IInterface {
			struct VMT : Antilatency::InterfaceContract::IInterface::VMT {
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getState(Antilatency::HardwareExtensionInterface::Interop::PinState& result) = 0;
				static constexpr Antilatency::InterfaceContract::InterfaceID ID() {
					return Antilatency::InterfaceContract::InterfaceID{0x39e6527d,0x82db,0x4615,{0xaf,0x8f,0x0f,0xad,0x4c,0x79,0xf1,0x5e}};
				}
			private:
				~VMT() = delete;
			};
			IInputPin() = default;
			IInputPin(std::nullptr_t) {}
			explicit IInputPin(VMT* pointer) : Antilatency::InterfaceContract::IInterface(pointer) {}
			template<typename T, typename = typename std::enable_if<std::is_base_of<IInputPin, T>::value>::type>
			IInputPin& operator = (const T& other) {
			    Antilatency::InterfaceContract::IInterface::operator=(other);
			    return *this;
			}
			template<class Implementer, class ... TArgs>
			static IInputPin create(TArgs&&... args) {
			    return *new Implementer(std::forward<TArgs>(args)...);
			}
			void attach(VMT* other) ANTILATENCY_NOEXCEPT {
			    Antilatency::InterfaceContract::IInterface::attach(other);
			}
			VMT* detach() ANTILATENCY_NOEXCEPT {
			    return reinterpret_cast<VMT*>(Antilatency::InterfaceContract::IInterface::detach());
			}
			/// <summary>Read latest state.</summary>
			Antilatency::HardwareExtensionInterface::Interop::PinState getState() {
				Antilatency::HardwareExtensionInterface::Interop::PinState result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->getState(result));
				return result;
			}
		};
	} //namespace HardwareExtensionInterface
} //namespace Antilatency
namespace Antilatency {
	namespace InterfaceContract {
		namespace Details {
			template<typename Implementer, typename LifeTimeController>
			struct InterfaceRemap<Antilatency::HardwareExtensionInterface::IInputPin, Implementer, LifeTimeController> : Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController> {
			public:
			    InterfaceRemap() = default;
			    static bool isInterfaceSupported(const Antilatency::InterfaceContract::InterfaceID& id) {
					if (id == Antilatency::HardwareExtensionInterface::IInputPin::VMT::ID()) {
						return true;
					}
					return Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController>::isInterfaceSupported(id);
				}
			public:
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getState(Antilatency::HardwareExtensionInterface::Interop::PinState& result) {
					try {
						result = this->_object->getState();
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
	namespace HardwareExtensionInterface {
		/* copy and paste this to implementer
		void setState(Antilatency::HardwareExtensionInterface::Interop::PinState state) {
			throw std::logic_error{"Method IOutputPin.setState() is not implemented."};
		}
		Antilatency::HardwareExtensionInterface::Interop::PinState getState() {
			throw std::logic_error{"Method IOutputPin.getState() is not implemented."};
		}
		*/
		/// <summary>Interface for controlling state of pin in output mode.</summary>
		struct IOutputPin : Antilatency::InterfaceContract::IInterface {
			struct VMT : Antilatency::InterfaceContract::IInterface::VMT {
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL setState(Antilatency::HardwareExtensionInterface::Interop::PinState state) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getState(Antilatency::HardwareExtensionInterface::Interop::PinState& result) = 0;
				static constexpr Antilatency::InterfaceContract::InterfaceID ID() {
					return Antilatency::InterfaceContract::InterfaceID{0x20d12574,0x3ae9,0x4921,{0x80,0xd3,0x95,0x21,0x7e,0x61,0xf4,0xb2}};
				}
			private:
				~VMT() = delete;
			};
			IOutputPin() = default;
			IOutputPin(std::nullptr_t) {}
			explicit IOutputPin(VMT* pointer) : Antilatency::InterfaceContract::IInterface(pointer) {}
			template<typename T, typename = typename std::enable_if<std::is_base_of<IOutputPin, T>::value>::type>
			IOutputPin& operator = (const T& other) {
			    Antilatency::InterfaceContract::IInterface::operator=(other);
			    return *this;
			}
			template<class Implementer, class ... TArgs>
			static IOutputPin create(TArgs&&... args) {
			    return *new Implementer(std::forward<TArgs>(args)...);
			}
			void attach(VMT* other) ANTILATENCY_NOEXCEPT {
			    Antilatency::InterfaceContract::IInterface::attach(other);
			}
			VMT* detach() ANTILATENCY_NOEXCEPT {
			    return reinterpret_cast<VMT*>(Antilatency::InterfaceContract::IInterface::detach());
			}
			/// <summary>Set logic level on pin.</summary>
			void setState(Antilatency::HardwareExtensionInterface::Interop::PinState state) {
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->setState(state));
			}
			/// <summary>Read latest state(cashed).</summary>
			Antilatency::HardwareExtensionInterface::Interop::PinState getState() {
				Antilatency::HardwareExtensionInterface::Interop::PinState result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->getState(result));
				return result;
			}
		};
	} //namespace HardwareExtensionInterface
} //namespace Antilatency
namespace Antilatency {
	namespace InterfaceContract {
		namespace Details {
			template<typename Implementer, typename LifeTimeController>
			struct InterfaceRemap<Antilatency::HardwareExtensionInterface::IOutputPin, Implementer, LifeTimeController> : Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController> {
			public:
			    InterfaceRemap() = default;
			    static bool isInterfaceSupported(const Antilatency::InterfaceContract::InterfaceID& id) {
					if (id == Antilatency::HardwareExtensionInterface::IOutputPin::VMT::ID()) {
						return true;
					}
					return Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController>::isInterfaceSupported(id);
				}
			public:
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL setState(Antilatency::HardwareExtensionInterface::Interop::PinState state) {
					try {
						this->_object->setState(state);
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getState(Antilatency::HardwareExtensionInterface::Interop::PinState& result) {
					try {
						result = this->_object->getState();
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
	namespace HardwareExtensionInterface {
		/* copy and paste this to implementer
		float getValue() {
			throw std::logic_error{"Method IAnalogPin.getValue() is not implemented."};
		}
		*/
		/// <summary>Interface for reading state of pin in analog mode.</summary>
		struct IAnalogPin : Antilatency::InterfaceContract::IInterface {
			struct VMT : Antilatency::InterfaceContract::IInterface::VMT {
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getValue(float& result) = 0;
				static constexpr Antilatency::InterfaceContract::InterfaceID ID() {
					return Antilatency::InterfaceContract::InterfaceID{0xe927972f,0x10e5,0x43e2,{0x95,0x02,0x74,0xbc,0xfe,0xd3,0x24,0x82}};
				}
			private:
				~VMT() = delete;
			};
			IAnalogPin() = default;
			IAnalogPin(std::nullptr_t) {}
			explicit IAnalogPin(VMT* pointer) : Antilatency::InterfaceContract::IInterface(pointer) {}
			template<typename T, typename = typename std::enable_if<std::is_base_of<IAnalogPin, T>::value>::type>
			IAnalogPin& operator = (const T& other) {
			    Antilatency::InterfaceContract::IInterface::operator=(other);
			    return *this;
			}
			template<class Implementer, class ... TArgs>
			static IAnalogPin create(TArgs&&... args) {
			    return *new Implementer(std::forward<TArgs>(args)...);
			}
			void attach(VMT* other) ANTILATENCY_NOEXCEPT {
			    Antilatency::InterfaceContract::IInterface::attach(other);
			}
			VMT* detach() ANTILATENCY_NOEXCEPT {
			    return reinterpret_cast<VMT*>(Antilatency::InterfaceContract::IInterface::detach());
			}
			/// <summary>Read latest voltage.</summary>
			/// <returns>Voltage in volts.</returns>
			float getValue() {
				float result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->getValue(result));
				return result;
			}
		};
	} //namespace HardwareExtensionInterface
} //namespace Antilatency
namespace Antilatency {
	namespace InterfaceContract {
		namespace Details {
			template<typename Implementer, typename LifeTimeController>
			struct InterfaceRemap<Antilatency::HardwareExtensionInterface::IAnalogPin, Implementer, LifeTimeController> : Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController> {
			public:
			    InterfaceRemap() = default;
			    static bool isInterfaceSupported(const Antilatency::InterfaceContract::InterfaceID& id) {
					if (id == Antilatency::HardwareExtensionInterface::IAnalogPin::VMT::ID()) {
						return true;
					}
					return Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController>::isInterfaceSupported(id);
				}
			public:
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getValue(float& result) {
					try {
						result = this->_object->getValue();
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
	namespace HardwareExtensionInterface {
		/* copy and paste this to implementer
		uint16_t getValue() {
			throw std::logic_error{"Method IPulseCounterPin.getValue() is not implemented."};
		}
		*/
		/// <summary>Interface for reading a count of rising edges for pin in pulse counter mode.</summary>
		struct IPulseCounterPin : Antilatency::InterfaceContract::IInterface {
			struct VMT : Antilatency::InterfaceContract::IInterface::VMT {
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getValue(uint16_t& result) = 0;
				static constexpr Antilatency::InterfaceContract::InterfaceID ID() {
					return Antilatency::InterfaceContract::InterfaceID{0x8998b746,0x5d17,0x4a47,{0x9c,0x8b,0x01,0xd5,0xa9,0x59,0x53,0x6c}};
				}
			private:
				~VMT() = delete;
			};
			IPulseCounterPin() = default;
			IPulseCounterPin(std::nullptr_t) {}
			explicit IPulseCounterPin(VMT* pointer) : Antilatency::InterfaceContract::IInterface(pointer) {}
			template<typename T, typename = typename std::enable_if<std::is_base_of<IPulseCounterPin, T>::value>::type>
			IPulseCounterPin& operator = (const T& other) {
			    Antilatency::InterfaceContract::IInterface::operator=(other);
			    return *this;
			}
			template<class Implementer, class ... TArgs>
			static IPulseCounterPin create(TArgs&&... args) {
			    return *new Implementer(std::forward<TArgs>(args)...);
			}
			void attach(VMT* other) ANTILATENCY_NOEXCEPT {
			    Antilatency::InterfaceContract::IInterface::attach(other);
			}
			VMT* detach() ANTILATENCY_NOEXCEPT {
			    return reinterpret_cast<VMT*>(Antilatency::InterfaceContract::IInterface::detach());
			}
			/// <summary>Read latest count of rising edges.</summary>
			uint16_t getValue() {
				uint16_t result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->getValue(result));
				return result;
			}
		};
	} //namespace HardwareExtensionInterface
} //namespace Antilatency
namespace Antilatency {
	namespace InterfaceContract {
		namespace Details {
			template<typename Implementer, typename LifeTimeController>
			struct InterfaceRemap<Antilatency::HardwareExtensionInterface::IPulseCounterPin, Implementer, LifeTimeController> : Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController> {
			public:
			    InterfaceRemap() = default;
			    static bool isInterfaceSupported(const Antilatency::InterfaceContract::InterfaceID& id) {
					if (id == Antilatency::HardwareExtensionInterface::IPulseCounterPin::VMT::ID()) {
						return true;
					}
					return Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController>::isInterfaceSupported(id);
				}
			public:
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getValue(uint16_t& result) {
					try {
						result = this->_object->getValue();
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
	namespace HardwareExtensionInterface {
		/* copy and paste this to implementer
		void setDuty(float value) {
			throw std::logic_error{"Method IPwmPin.setDuty() is not implemented."};
		}
		float getDuty() {
			throw std::logic_error{"Method IPwmPin.getDuty() is not implemented."};
		}
		uint32_t getFrequency() {
			throw std::logic_error{"Method IPwmPin.getFrequency() is not implemented."};
		}
		*/
		/// <summary>Interface for controlling state of pin in pwm mode.</summary>
		struct IPwmPin : Antilatency::InterfaceContract::IInterface {
			struct VMT : Antilatency::InterfaceContract::IInterface::VMT {
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL setDuty(float value) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getDuty(float& result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getFrequency(uint32_t& result) = 0;
				static constexpr Antilatency::InterfaceContract::InterfaceID ID() {
					return Antilatency::InterfaceContract::InterfaceID{0x0793b2f5,0x2f6f,0x4a01,{0x8b,0x25,0xf2,0xff,0x98,0x36,0x04,0x41}};
				}
			private:
				~VMT() = delete;
			};
			IPwmPin() = default;
			IPwmPin(std::nullptr_t) {}
			explicit IPwmPin(VMT* pointer) : Antilatency::InterfaceContract::IInterface(pointer) {}
			template<typename T, typename = typename std::enable_if<std::is_base_of<IPwmPin, T>::value>::type>
			IPwmPin& operator = (const T& other) {
			    Antilatency::InterfaceContract::IInterface::operator=(other);
			    return *this;
			}
			template<class Implementer, class ... TArgs>
			static IPwmPin create(TArgs&&... args) {
			    return *new Implementer(std::forward<TArgs>(args)...);
			}
			void attach(VMT* other) ANTILATENCY_NOEXCEPT {
			    Antilatency::InterfaceContract::IInterface::attach(other);
			}
			VMT* detach() ANTILATENCY_NOEXCEPT {
			    return reinterpret_cast<VMT*>(Antilatency::InterfaceContract::IInterface::detach());
			}
			/// <summary>Set pwm duty.</summary>
			/// <param name = "value">
			/// Target duty in range [0;1].
			/// </param>
			void setDuty(float value) {
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->setDuty(value));
			}
			/// <summary>Get actual pwm duty.</summary>
			/// <returns>Pwm duty in range [0;1].</returns>
			float getDuty() {
				float result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->getDuty(result));
				return result;
			}
			/// <summary>Get actual pwm frequency.</summary>
			/// <returns>Pwm frequency in hertz.</returns>
			uint32_t getFrequency() {
				uint32_t result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->getFrequency(result));
				return result;
			}
		};
	} //namespace HardwareExtensionInterface
} //namespace Antilatency
namespace Antilatency {
	namespace InterfaceContract {
		namespace Details {
			template<typename Implementer, typename LifeTimeController>
			struct InterfaceRemap<Antilatency::HardwareExtensionInterface::IPwmPin, Implementer, LifeTimeController> : Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController> {
			public:
			    InterfaceRemap() = default;
			    static bool isInterfaceSupported(const Antilatency::InterfaceContract::InterfaceID& id) {
					if (id == Antilatency::HardwareExtensionInterface::IPwmPin::VMT::ID()) {
						return true;
					}
					return Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController>::isInterfaceSupported(id);
				}
			public:
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL setDuty(float value) {
					try {
						this->_object->setDuty(value);
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getDuty(float& result) {
					try {
						result = this->_object->getDuty();
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getFrequency(uint32_t& result) {
					try {
						result = this->_object->getFrequency();
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
	namespace HardwareExtensionInterface {
		/* copy and paste this to implementer
		Antilatency::HardwareExtensionInterface::IInputPin createInputPin(Antilatency::HardwareExtensionInterface::Interop::Pins pin) {
			throw std::logic_error{"Method ICotask.createInputPin() is not implemented."};
		}
		Antilatency::HardwareExtensionInterface::IOutputPin createOutputPin(Antilatency::HardwareExtensionInterface::Interop::Pins pin, Antilatency::HardwareExtensionInterface::Interop::PinState initialState) {
			throw std::logic_error{"Method ICotask.createOutputPin() is not implemented."};
		}
		Antilatency::HardwareExtensionInterface::IAnalogPin createAnalogPin(Antilatency::HardwareExtensionInterface::Interop::Pins pin, uint32_t refreshIntervalMs) {
			throw std::logic_error{"Method ICotask.createAnalogPin() is not implemented."};
		}
		Antilatency::HardwareExtensionInterface::IPulseCounterPin createPulseCounterPin(Antilatency::HardwareExtensionInterface::Interop::Pins pin, uint32_t refreshIntervalMs) {
			throw std::logic_error{"Method ICotask.createPulseCounterPin() is not implemented."};
		}
		Antilatency::HardwareExtensionInterface::IPwmPin createPwmPin(Antilatency::HardwareExtensionInterface::Interop::Pins pin, uint32_t frequency, float initialDuty) {
			throw std::logic_error{"Method ICotask.createPwmPin() is not implemented."};
		}
		void run() {
			throw std::logic_error{"Method ICotask.run() is not implemented."};
		}
		*/
		struct ICotask : Antilatency::DeviceNetwork::ICotask {
			struct VMT : Antilatency::DeviceNetwork::ICotask::VMT {
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL createInputPin(Antilatency::HardwareExtensionInterface::Interop::Pins pin, Antilatency::HardwareExtensionInterface::IInputPin& result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL createOutputPin(Antilatency::HardwareExtensionInterface::Interop::Pins pin, Antilatency::HardwareExtensionInterface::Interop::PinState initialState, Antilatency::HardwareExtensionInterface::IOutputPin& result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL createAnalogPin(Antilatency::HardwareExtensionInterface::Interop::Pins pin, uint32_t refreshIntervalMs, Antilatency::HardwareExtensionInterface::IAnalogPin& result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL createPulseCounterPin(Antilatency::HardwareExtensionInterface::Interop::Pins pin, uint32_t refreshIntervalMs, Antilatency::HardwareExtensionInterface::IPulseCounterPin& result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL createPwmPin(Antilatency::HardwareExtensionInterface::Interop::Pins pin, uint32_t frequency, float initialDuty, Antilatency::HardwareExtensionInterface::IPwmPin& result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL run() = 0;
				static constexpr Antilatency::InterfaceContract::InterfaceID ID() {
					return Antilatency::InterfaceContract::InterfaceID{0xacd1daa9,0x9394,0x4d3a,{0x95,0xce,0x17,0x7a,0x23,0xbd,0x9b,0x89}};
				}
			private:
				~VMT() = delete;
			};
			ICotask() = default;
			ICotask(std::nullptr_t) {}
			explicit ICotask(VMT* pointer) : Antilatency::DeviceNetwork::ICotask(pointer) {}
			template<typename T, typename = typename std::enable_if<std::is_base_of<ICotask, T>::value>::type>
			ICotask& operator = (const T& other) {
			    Antilatency::DeviceNetwork::ICotask::operator=(other);
			    return *this;
			}
			template<class Implementer, class ... TArgs>
			static ICotask create(TArgs&&... args) {
			    return *new Implementer(std::forward<TArgs>(args)...);
			}
			void attach(VMT* other) ANTILATENCY_NOEXCEPT {
			    Antilatency::DeviceNetwork::ICotask::attach(other);
			}
			VMT* detach() ANTILATENCY_NOEXCEPT {
			    return reinterpret_cast<VMT*>(Antilatency::DeviceNetwork::ICotask::detach());
			}
			/// <summary>Create InputPin for reading state of pin.</summary>
			/// <param name = "pin">
			/// Target pin.
			/// </param>
			Antilatency::HardwareExtensionInterface::IInputPin createInputPin(Antilatency::HardwareExtensionInterface::Interop::Pins pin) {
				Antilatency::HardwareExtensionInterface::IInputPin result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->createInputPin(pin, result));
				return result;
			}
			/// <summary>Create OutputPin for controlling state of pin.</summary>
			/// <param name = "pin">
			/// Target pin.
			/// </param>
			/// <param name = "initialState">
			/// Logic level on pin right after init.
			/// </param>
			Antilatency::HardwareExtensionInterface::IOutputPin createOutputPin(Antilatency::HardwareExtensionInterface::Interop::Pins pin, Antilatency::HardwareExtensionInterface::Interop::PinState initialState) {
				Antilatency::HardwareExtensionInterface::IOutputPin result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->createOutputPin(pin, initialState, result));
				return result;
			}
			/// <summary>Create AnalogPin for reading voltage on pin.</summary>
			/// <param name = "pin">
			/// Target pin.
			/// </param>
			/// <param name = "refreshIntervalMs">
			/// Interval(in ms) of value updating.
			/// </param>
			Antilatency::HardwareExtensionInterface::IAnalogPin createAnalogPin(Antilatency::HardwareExtensionInterface::Interop::Pins pin, uint32_t refreshIntervalMs) {
				Antilatency::HardwareExtensionInterface::IAnalogPin result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->createAnalogPin(pin, refreshIntervalMs, result));
				return result;
			}
			/// <summary>Create PulseCounter for counting rising edges on pin.</summary>
			/// <param name = "pin">
			/// Target pin.
			/// </param>
			/// <param name = "refreshIntervalMs">
			/// Interval(in ms) of value updating.
			/// </param>
			Antilatency::HardwareExtensionInterface::IPulseCounterPin createPulseCounterPin(Antilatency::HardwareExtensionInterface::Interop::Pins pin, uint32_t refreshIntervalMs) {
				Antilatency::HardwareExtensionInterface::IPulseCounterPin result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->createPulseCounterPin(pin, refreshIntervalMs, result));
				return result;
			}
			/// <summary>Create PwmPin for controlling state of pwm pin.</summary>
			/// <param name = "pin">
			/// Target pin.
			/// </param>
			/// <param name = "frequency">
			/// Target frequency in hertz. Should be equal for all pwm pins.
			/// </param>
			/// <param name = "initialDuty">
			/// Pwm duty right after init.
			/// </param>
			Antilatency::HardwareExtensionInterface::IPwmPin createPwmPin(Antilatency::HardwareExtensionInterface::Interop::Pins pin, uint32_t frequency, float initialDuty) {
				Antilatency::HardwareExtensionInterface::IPwmPin result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->createPwmPin(pin, frequency, initialDuty, result));
				return result;
			}
			/// <summary>Apply config and start processing of pin states. Switch task in Run mode(configuring not possible).</summary>
			void run() {
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->run());
			}
		};
	} //namespace HardwareExtensionInterface
} //namespace Antilatency
namespace Antilatency {
	namespace InterfaceContract {
		namespace Details {
			template<typename Implementer, typename LifeTimeController>
			struct InterfaceRemap<Antilatency::HardwareExtensionInterface::ICotask, Implementer, LifeTimeController> : Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::DeviceNetwork::ICotask, Implementer, LifeTimeController> {
			public:
			    InterfaceRemap() = default;
			    static bool isInterfaceSupported(const Antilatency::InterfaceContract::InterfaceID& id) {
					if (id == Antilatency::HardwareExtensionInterface::ICotask::VMT::ID()) {
						return true;
					}
					return Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::DeviceNetwork::ICotask, Implementer, LifeTimeController>::isInterfaceSupported(id);
				}
			public:
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL createInputPin(Antilatency::HardwareExtensionInterface::Interop::Pins pin, Antilatency::HardwareExtensionInterface::IInputPin& result) {
					try {
						result = this->_object->createInputPin(pin);
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL createOutputPin(Antilatency::HardwareExtensionInterface::Interop::Pins pin, Antilatency::HardwareExtensionInterface::Interop::PinState initialState, Antilatency::HardwareExtensionInterface::IOutputPin& result) {
					try {
						result = this->_object->createOutputPin(pin, initialState);
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL createAnalogPin(Antilatency::HardwareExtensionInterface::Interop::Pins pin, uint32_t refreshIntervalMs, Antilatency::HardwareExtensionInterface::IAnalogPin& result) {
					try {
						result = this->_object->createAnalogPin(pin, refreshIntervalMs);
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL createPulseCounterPin(Antilatency::HardwareExtensionInterface::Interop::Pins pin, uint32_t refreshIntervalMs, Antilatency::HardwareExtensionInterface::IPulseCounterPin& result) {
					try {
						result = this->_object->createPulseCounterPin(pin, refreshIntervalMs);
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL createPwmPin(Antilatency::HardwareExtensionInterface::Interop::Pins pin, uint32_t frequency, float initialDuty, Antilatency::HardwareExtensionInterface::IPwmPin& result) {
					try {
						result = this->_object->createPwmPin(pin, frequency, initialDuty);
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL run() {
					try {
						this->_object->run();
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
	namespace HardwareExtensionInterface {
		/* copy and paste this to implementer
		Antilatency::HardwareExtensionInterface::ICotask startTask(Antilatency::DeviceNetwork::INetwork network, Antilatency::DeviceNetwork::NodeHandle node) {
			throw std::logic_error{"Method ICotaskConstructor.startTask() is not implemented."};
		}
		*/
		/// <summary>CotaskConstructor for task laucnhing.</summary>
		struct ICotaskConstructor : Antilatency::DeviceNetwork::ICotaskConstructor {
			struct VMT : Antilatency::DeviceNetwork::ICotaskConstructor::VMT {
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL startTask(Antilatency::DeviceNetwork::INetwork::VMT* network, Antilatency::DeviceNetwork::NodeHandle node, Antilatency::HardwareExtensionInterface::ICotask& result) = 0;
				static constexpr Antilatency::InterfaceContract::InterfaceID ID() {
					return Antilatency::InterfaceContract::InterfaceID{0x53b72a7a,0xc61e,0x4946,{0x8a,0x06,0x56,0x4a,0x71,0x71,0xd4,0x77}};
				}
			private:
				~VMT() = delete;
			};
			ICotaskConstructor() = default;
			ICotaskConstructor(std::nullptr_t) {}
			explicit ICotaskConstructor(VMT* pointer) : Antilatency::DeviceNetwork::ICotaskConstructor(pointer) {}
			template<typename T, typename = typename std::enable_if<std::is_base_of<ICotaskConstructor, T>::value>::type>
			ICotaskConstructor& operator = (const T& other) {
			    Antilatency::DeviceNetwork::ICotaskConstructor::operator=(other);
			    return *this;
			}
			template<class Implementer, class ... TArgs>
			static ICotaskConstructor create(TArgs&&... args) {
			    return *new Implementer(std::forward<TArgs>(args)...);
			}
			void attach(VMT* other) ANTILATENCY_NOEXCEPT {
			    Antilatency::DeviceNetwork::ICotaskConstructor::attach(other);
			}
			VMT* detach() ANTILATENCY_NOEXCEPT {
			    return reinterpret_cast<VMT*>(Antilatency::DeviceNetwork::ICotaskConstructor::detach());
			}
			/// <summary>Start task to get access for metrics.</summary>
			Antilatency::HardwareExtensionInterface::ICotask startTask(Antilatency::DeviceNetwork::INetwork network, Antilatency::DeviceNetwork::NodeHandle node) {
				Antilatency::HardwareExtensionInterface::ICotask result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->startTask(network.detach(), node, result));
				return result;
			}
		};
	} //namespace HardwareExtensionInterface
} //namespace Antilatency
namespace Antilatency {
	namespace InterfaceContract {
		namespace Details {
			template<typename Implementer, typename LifeTimeController>
			struct InterfaceRemap<Antilatency::HardwareExtensionInterface::ICotaskConstructor, Implementer, LifeTimeController> : Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::DeviceNetwork::ICotaskConstructor, Implementer, LifeTimeController> {
			public:
			    InterfaceRemap() = default;
			    static bool isInterfaceSupported(const Antilatency::InterfaceContract::InterfaceID& id) {
					if (id == Antilatency::HardwareExtensionInterface::ICotaskConstructor::VMT::ID()) {
						return true;
					}
					return Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::DeviceNetwork::ICotaskConstructor, Implementer, LifeTimeController>::isInterfaceSupported(id);
				}
			public:
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL startTask(Antilatency::DeviceNetwork::INetwork::VMT* network, Antilatency::DeviceNetwork::NodeHandle node, Antilatency::HardwareExtensionInterface::ICotask& result) {
					try {
						Antilatency::DeviceNetwork::INetwork networkMarshaler;
						networkMarshaler.attach(network);
						result = this->_object->startTask(networkMarshaler, node);
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
	namespace HardwareExtensionInterface {
		/* copy and paste this to implementer
		std::string getVersion() {
			throw std::logic_error{"Method ILibrary.getVersion() is not implemented."};
		}
		Antilatency::HardwareExtensionInterface::ICotaskConstructor getCotaskConstructor() {
			throw std::logic_error{"Method ILibrary.getCotaskConstructor() is not implemented."};
		}
		*/
		/// <summary>AntilatencyHardwareExtensionInterface library entry point.</summary>
		struct ILibrary : Antilatency::InterfaceContract::IInterface {
			struct VMT : Antilatency::InterfaceContract::IInterface::VMT {
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getVersion(Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getCotaskConstructor(Antilatency::HardwareExtensionInterface::ICotaskConstructor& result) = 0;
				static constexpr Antilatency::InterfaceContract::InterfaceID ID() {
					return Antilatency::InterfaceContract::InterfaceID{0xef1b555b,0xbbf4,0x4514,{0x82,0x9b,0xb0,0xcd,0x15,0xb6,0xfc,0x8d}};
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
			/// <summary>Get version of AntilatencyHardwareExtensionInterface library.</summary>
			std::string getVersion() {
				std::string result;
				auto resultMarshaler = Antilatency::InterfaceContract::Details::ArrayOutMarshaler::create(result);
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->getVersion(resultMarshaler));
				return result;
			}
			/// <summary>Create AntilatencyHardwareExtensionInterface CotaskConstructor.</summary>
			Antilatency::HardwareExtensionInterface::ICotaskConstructor getCotaskConstructor() {
				Antilatency::HardwareExtensionInterface::ICotaskConstructor result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->getCotaskConstructor(result));
				return result;
			}
		};
	} //namespace HardwareExtensionInterface
} //namespace Antilatency
namespace Antilatency {
	namespace InterfaceContract {
		namespace Details {
			template<typename Implementer, typename LifeTimeController>
			struct InterfaceRemap<Antilatency::HardwareExtensionInterface::ILibrary, Implementer, LifeTimeController> : Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController> {
			public:
			    InterfaceRemap() = default;
			    static bool isInterfaceSupported(const Antilatency::InterfaceContract::InterfaceID& id) {
					if (id == Antilatency::HardwareExtensionInterface::ILibrary::VMT::ID()) {
						return true;
					}
					return Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController>::isInterfaceSupported(id);
				}
			public:
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getVersion(Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate result) {
					try {
						result = this->_object->getVersion();
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getCotaskConstructor(Antilatency::HardwareExtensionInterface::ICotaskConstructor& result) {
					try {
						result = this->_object->getCotaskConstructor();
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
	namespace HardwareExtensionInterface {
		namespace Constants {
			constexpr uint32_t AnalogMinRefreshIntervalMs = 1U;
			constexpr uint32_t AnalogMaxRefreshIntervalMs = 65535U;
			constexpr uint32_t PulseCounterMinRefreshIntervalMs = 1U;
			constexpr uint32_t PulseCounterMaxRefreshIntervalMs = 125U;
			constexpr uint32_t PwmMinFrequency = 20U;
			constexpr uint32_t PwmMaxFrequency = 10000U;
		} //namespace Constants
	} //namespace HardwareExtensionInterface
} //namespace Antilatency


#ifdef _MSC_VER
#pragma warning(pop)
#endif
