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
#include <cstdint>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4946)
#pragma warning(disable: 4265)
#endif
namespace Antilatency {
	namespace Bracer {
		/// <summary>Vibration parameters.</summary>
		struct Vibration {
			/// <summary>Vibration intensity value in range 0 ... 1. Value 0 - no vibration, value 1 - max vibration.</summary>
			float intensity;
			/// <summary>Vibration duration in seconds. Valid range 0.0 ... 65.535</summary>
			float duration;
		};
	} //namespace Bracer
} //namespace Antilatency

namespace Antilatency {
	namespace Bracer {
		/* copy and paste this to implementer
		uint32_t getTouchChannelsCount() {
			throw std::logic_error{"Method ICotask.getTouchChannelsCount() is not implemented."};
		}
		uint32_t getTouchNativeValue(uint32_t channel) {
			throw std::logic_error{"Method ICotask.getTouchNativeValue() is not implemented."};
		}
		float getTouch(uint32_t channel) {
			throw std::logic_error{"Method ICotask.getTouch() is not implemented."};
		}
		void executeVibrationSequence(const std::vector<Antilatency::Bracer::Vibration>& sequence) {
			throw std::logic_error{"Method ICotask.executeVibrationSequence() is not implemented."};
		}
		*/
		struct ICotask : Antilatency::DeviceNetwork::ICotaskBatteryPowered {
			struct VMT : Antilatency::DeviceNetwork::ICotaskBatteryPowered::VMT {
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getTouchChannelsCount(uint32_t& result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getTouchNativeValue(uint32_t channel, uint32_t& result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getTouch(uint32_t channel, float& result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL executeVibrationSequence(Antilatency::InterfaceContract::Details::ArrayInMarshaler::Intermediate sequence) = 0;
				static constexpr Antilatency::InterfaceContract::InterfaceID ID() {
					return Antilatency::InterfaceContract::InterfaceID{0x36f3603d,0x32d8,0x4f36,{0x9e,0xbd,0xa0,0x6a,0x3f,0x18,0x84,0x66}};
				}
			private:
				~VMT() = delete;
			};
			ICotask() = default;
			ICotask(std::nullptr_t) {}
			explicit ICotask(VMT* pointer) : Antilatency::DeviceNetwork::ICotaskBatteryPowered(pointer) {}
			template<typename T, typename = typename std::enable_if<std::is_base_of<ICotask, T>::value>::type>
			ICotask& operator = (const T& other) {
			    Antilatency::DeviceNetwork::ICotaskBatteryPowered::operator=(other);
			    return *this;
			}
			template<class Implementer, class ... TArgs>
			static ICotask create(TArgs&&... args) {
			    return *new Implementer(std::forward<TArgs>(args)...);
			}
			void attach(VMT* other) ANTILATENCY_NOEXCEPT {
			    Antilatency::DeviceNetwork::ICotaskBatteryPowered::attach(other);
			}
			VMT* detach() ANTILATENCY_NOEXCEPT {
			    return reinterpret_cast<VMT*>(Antilatency::DeviceNetwork::ICotaskBatteryPowered::detach());
			}
			/// <summary>Get count of touch channels.</summary>
			uint32_t getTouchChannelsCount() {
				uint32_t result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->getTouchChannelsCount(result));
				return result;
			}
			/// <summary>Get actual native touch value.</summary>
			/// <returns>Native touch value without any preprocessing. Lower value show stronger touch.</returns>
			/// <param name = "channel">
			/// Index of channel in range 0 .. (getTouchChannelsCount() - 1).
			/// </param>
			uint32_t getTouchNativeValue(uint32_t channel) {
				uint32_t result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->getTouchNativeValue(channel, result));
				return result;
			}
			/// <summary>Get actual touch value. Window for calculation based on values from device properties: "touch/WindowN. Where N - index of channel.</summary>
			/// <param name = "channel">
			/// Index of channel in range 0 .. (getTouchChannelsCount() - 1).
			/// </param>
			/// <returns>Touch value after processing in range 0 .. 1. Value 0 - no touch, value 1 - max touch. If native value is outside of windows, it will be clamped to range 0 .. 1.</returns>
			float getTouch(uint32_t channel) {
				float result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->getTouch(channel, result));
				return result;
			}
			/// <summary>Play continuous vibration sequence. Note, that You can use 0 intensity for delay in sequence.</summary>
			/// <param name = "sequence">
			/// Array of vibration parameters such intensity and duration. If values in sequence are outside valid range, method will throw invalid argument exception.
			/// </param>
			void executeVibrationSequence(const std::vector<Antilatency::Bracer::Vibration>& sequence) {
				auto sequenceMarshaler = Antilatency::InterfaceContract::Details::ArrayInMarshaler::create(sequence);
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->executeVibrationSequence(sequenceMarshaler));
			}
		};
	} //namespace Bracer
} //namespace Antilatency
namespace Antilatency {
	namespace InterfaceContract {
		namespace Details {
			template<typename Implementer, typename LifeTimeController>
			struct InterfaceRemap<Antilatency::Bracer::ICotask, Implementer, LifeTimeController> : Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::DeviceNetwork::ICotaskBatteryPowered, Implementer, LifeTimeController> {
			public:
			    InterfaceRemap() = default;
			    static bool isInterfaceSupported(const Antilatency::InterfaceContract::InterfaceID& id) {
					if (id == Antilatency::Bracer::ICotask::VMT::ID()) {
						return true;
					}
					return Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::DeviceNetwork::ICotaskBatteryPowered, Implementer, LifeTimeController>::isInterfaceSupported(id);
				}
			public:
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getTouchChannelsCount(uint32_t& result) {
					try {
						result = this->_object->getTouchChannelsCount();
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getTouchNativeValue(uint32_t channel, uint32_t& result) {
					try {
						result = this->_object->getTouchNativeValue(channel);
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getTouch(uint32_t channel, float& result) {
					try {
						result = this->_object->getTouch(channel);
					}
					catch (...) {
						return Antilatency::InterfaceContract::handleRemapException(this->_object);
					}
					return Antilatency::InterfaceContract::ExceptionCode::Ok;
				}
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL executeVibrationSequence(Antilatency::InterfaceContract::Details::ArrayInMarshaler::Intermediate sequence) {
					try {
						this->_object->executeVibrationSequence(sequence);
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
	namespace Bracer {
		/* copy and paste this to implementer
		Antilatency::Bracer::ICotask startTask(Antilatency::DeviceNetwork::INetwork network, Antilatency::DeviceNetwork::NodeHandle node) {
			throw std::logic_error{"Method ICotaskConstructor.startTask() is not implemented."};
		}
		*/
		struct ICotaskConstructor : Antilatency::DeviceNetwork::ICotaskConstructor {
			struct VMT : Antilatency::DeviceNetwork::ICotaskConstructor::VMT {
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL startTask(Antilatency::DeviceNetwork::INetwork::VMT* network, Antilatency::DeviceNetwork::NodeHandle node, Antilatency::Bracer::ICotask& result) = 0;
				static constexpr Antilatency::InterfaceContract::InterfaceID ID() {
					return Antilatency::InterfaceContract::InterfaceID{0x28ab49e0,0xa878,0x4664,{0xb2,0x6e,0x65,0x43,0xf1,0xe1,0x1f,0x92}};
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
			Antilatency::Bracer::ICotask startTask(Antilatency::DeviceNetwork::INetwork network, Antilatency::DeviceNetwork::NodeHandle node) {
				Antilatency::Bracer::ICotask result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->startTask(network.detach(), node, result));
				return result;
			}
		};
	} //namespace Bracer
} //namespace Antilatency
namespace Antilatency {
	namespace InterfaceContract {
		namespace Details {
			template<typename Implementer, typename LifeTimeController>
			struct InterfaceRemap<Antilatency::Bracer::ICotaskConstructor, Implementer, LifeTimeController> : Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::DeviceNetwork::ICotaskConstructor, Implementer, LifeTimeController> {
			public:
			    InterfaceRemap() = default;
			    static bool isInterfaceSupported(const Antilatency::InterfaceContract::InterfaceID& id) {
					if (id == Antilatency::Bracer::ICotaskConstructor::VMT::ID()) {
						return true;
					}
					return Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::DeviceNetwork::ICotaskConstructor, Implementer, LifeTimeController>::isInterfaceSupported(id);
				}
			public:
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL startTask(Antilatency::DeviceNetwork::INetwork::VMT* network, Antilatency::DeviceNetwork::NodeHandle node, Antilatency::Bracer::ICotask& result) {
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
	namespace Bracer {
		/* copy and paste this to implementer
		std::string getVersion() {
			throw std::logic_error{"Method ILibrary.getVersion() is not implemented."};
		}
		Antilatency::Bracer::ICotaskConstructor getCotaskConstructor() {
			throw std::logic_error{"Method ILibrary.getCotaskConstructor() is not implemented."};
		}
		*/
		struct ILibrary : Antilatency::InterfaceContract::IInterface {
			struct VMT : Antilatency::InterfaceContract::IInterface::VMT {
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getVersion(Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate result) = 0;
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getCotaskConstructor(Antilatency::Bracer::ICotaskConstructor& result) = 0;
				static constexpr Antilatency::InterfaceContract::InterfaceID ID() {
					return Antilatency::InterfaceContract::InterfaceID{0x8c3ad766,0x5af7,0x4c13,{0xba,0xa1,0xe9,0x8c,0xbd,0xfa,0x67,0x1e}};
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
			/// <summary>Get version of AntilatencyBracer library.</summary>
			std::string getVersion() {
				std::string result;
				auto resultMarshaler = Antilatency::InterfaceContract::Details::ArrayOutMarshaler::create(result);
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->getVersion(resultMarshaler));
				return result;
			}
			/// <summary>Create AntilatencyBracer CotaskConstructor.</summary>
			Antilatency::Bracer::ICotaskConstructor getCotaskConstructor() {
				Antilatency::Bracer::ICotaskConstructor result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->getCotaskConstructor(result));
				return result;
			}
		};
	} //namespace Bracer
} //namespace Antilatency
namespace Antilatency {
	namespace InterfaceContract {
		namespace Details {
			template<typename Implementer, typename LifeTimeController>
			struct InterfaceRemap<Antilatency::Bracer::ILibrary, Implementer, LifeTimeController> : Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController> {
			public:
			    InterfaceRemap() = default;
			    static bool isInterfaceSupported(const Antilatency::InterfaceContract::InterfaceID& id) {
					if (id == Antilatency::Bracer::ILibrary::VMT::ID()) {
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
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getCotaskConstructor(Antilatency::Bracer::ICotaskConstructor& result) {
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
	namespace Bracer {
		namespace Constants {
			static constexpr const char* TouchChannelsCountPropertyName = "sys/TouchChannelsCount";
			static constexpr const char* TouchWindowBaseName = "touch/Window";
		} //namespace Constants
	} //namespace Bracer
} //namespace Antilatency


#ifdef _MSC_VER
#pragma warning(pop)
#endif
