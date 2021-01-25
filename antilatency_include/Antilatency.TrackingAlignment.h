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
#include <Antilatency.Math.h>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4946)
#pragma warning(disable: 4265)
#endif
namespace Antilatency {
	namespace TrackingAlignment {
		struct State {
			Antilatency::Math::doubleQ rotationARelativeToB;
			Antilatency::Math::doubleQ rotationBSpace;
			double timeBAheadOfA;
		};
	} //namespace TrackingAlignment
} //namespace Antilatency

namespace Antilatency {
	namespace TrackingAlignment {
		/* copy and paste this to implementer
		Antilatency::TrackingAlignment::State update(Antilatency::Math::doubleQ a, Antilatency::Math::doubleQ b, double time) {
			throw std::logic_error{"Method ITrackingAlignment.update() is not implemented."};
		}
		*/
		/// <summary>This thing allows to align Alt tracking rotation with third-party tracking, i.e. obtain such rotationBSpace, rotationARelativeToB quaternions, that a = rotationBSpace*b*rotationARelativeToB for a == Alt tracking rotation without extrapolation, b == third-party tracker's rotation as is</summary>
		struct ITrackingAlignment : Antilatency::InterfaceContract::IInterface {
			struct VMT : Antilatency::InterfaceContract::IInterface::VMT {
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL update(Antilatency::Math::doubleQ a, Antilatency::Math::doubleQ b, double time, Antilatency::TrackingAlignment::State& result) = 0;
				static constexpr Antilatency::InterfaceContract::InterfaceID ID() {
					return Antilatency::InterfaceContract::InterfaceID{0x502bf5a7,0x9d17,0x4419,{0xb5,0x75,0x97,0x3f,0x46,0x7e,0x13,0xd4}};
				}
			private:
				~VMT() = delete;
			};
			ITrackingAlignment() = default;
			ITrackingAlignment(std::nullptr_t) {}
			explicit ITrackingAlignment(VMT* pointer) : Antilatency::InterfaceContract::IInterface(pointer) {}
			template<typename T, typename = typename std::enable_if<std::is_base_of<ITrackingAlignment, T>::value>::type>
			ITrackingAlignment& operator = (const T& other) {
			    Antilatency::InterfaceContract::IInterface::operator=(other);
			    return *this;
			}
			template<class Implementer, class ... TArgs>
			static ITrackingAlignment create(TArgs&&... args) {
			    return *new Implementer(std::forward<TArgs>(args)...);
			}
			void attach(VMT* other) ANTILATENCY_NOEXCEPT {
			    Antilatency::InterfaceContract::IInterface::attach(other);
			}
			VMT* detach() ANTILATENCY_NOEXCEPT {
			    return reinterpret_cast<VMT*>(Antilatency::InterfaceContract::IInterface::detach());
			}
			/// <summary>Provide data to optimizer</summary>
			/// <param name = "a">
			/// Alt rotation without extrapolation
			/// </param>
			/// <param name = "b">
			/// Third-party tracker's rotation as is
			/// </param>
			/// <param name = "time">
			/// Current time in seconds
			/// </param>
			Antilatency::TrackingAlignment::State update(Antilatency::Math::doubleQ a, Antilatency::Math::doubleQ b, double time) {
				Antilatency::TrackingAlignment::State result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->update(a, b, time, result));
				return result;
			}
		};
	} //namespace TrackingAlignment
} //namespace Antilatency
namespace Antilatency {
	namespace InterfaceContract {
		namespace Details {
			template<typename Implementer, typename LifeTimeController>
			struct InterfaceRemap<Antilatency::TrackingAlignment::ITrackingAlignment, Implementer, LifeTimeController> : Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController> {
			public:
			    InterfaceRemap() = default;
			    static bool isInterfaceSupported(const Antilatency::InterfaceContract::InterfaceID& id) {
					if (id == Antilatency::TrackingAlignment::ITrackingAlignment::VMT::ID()) {
						return true;
					}
					return Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController>::isInterfaceSupported(id);
				}
			public:
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL update(Antilatency::Math::doubleQ a, Antilatency::Math::doubleQ b, double time, Antilatency::TrackingAlignment::State& result) {
					try {
						result = this->_object->update(a, b, time);
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
	namespace TrackingAlignment {
		/* copy and paste this to implementer
		Antilatency::TrackingAlignment::ITrackingAlignment createTrackingAlignment(Antilatency::Math::doubleQ initialARelativeToB, double initialTimeBAheadOfA) {
			throw std::logic_error{"Method ILibrary.createTrackingAlignment() is not implemented."};
		}
		*/
		struct ILibrary : Antilatency::InterfaceContract::IInterface {
			struct VMT : Antilatency::InterfaceContract::IInterface::VMT {
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL createTrackingAlignment(Antilatency::Math::doubleQ initialARelativeToB, double initialTimeBAheadOfA, Antilatency::TrackingAlignment::ITrackingAlignment& result) = 0;
				static constexpr Antilatency::InterfaceContract::InterfaceID ID() {
					return Antilatency::InterfaceContract::InterfaceID{0x5f53c3cf,0x8080,0x41f1,{0x87,0x69,0x49,0x92,0xc0,0xdb,0x36,0xea}};
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
			Antilatency::TrackingAlignment::ITrackingAlignment createTrackingAlignment(Antilatency::Math::doubleQ initialARelativeToB, double initialTimeBAheadOfA) {
				Antilatency::TrackingAlignment::ITrackingAlignment result;
				handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->createTrackingAlignment(initialARelativeToB, initialTimeBAheadOfA, result));
				return result;
			}
		};
	} //namespace TrackingAlignment
} //namespace Antilatency
namespace Antilatency {
	namespace InterfaceContract {
		namespace Details {
			template<typename Implementer, typename LifeTimeController>
			struct InterfaceRemap<Antilatency::TrackingAlignment::ILibrary, Implementer, LifeTimeController> : Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController> {
			public:
			    InterfaceRemap() = default;
			    static bool isInterfaceSupported(const Antilatency::InterfaceContract::InterfaceID& id) {
					if (id == Antilatency::TrackingAlignment::ILibrary::VMT::ID()) {
						return true;
					}
					return Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController>::isInterfaceSupported(id);
				}
			public:
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL createTrackingAlignment(Antilatency::Math::doubleQ initialARelativeToB, double initialTimeBAheadOfA, Antilatency::TrackingAlignment::ITrackingAlignment& result) {
					try {
						result = this->_object->createTrackingAlignment(initialARelativeToB, initialTimeBAheadOfA);
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
