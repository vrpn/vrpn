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
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4946)
#pragma warning(disable: 4265)
#endif
namespace AndroidJniWrapper {
	/* copy and paste this to implementer
	void initJni(void* vm, void* context) {
		throw std::logic_error{"Method IAndroidJni.initJni() is not implemented."};
	}
	*/
	struct IAndroidJni : Antilatency::InterfaceContract::IInterface {
		struct VMT : Antilatency::InterfaceContract::IInterface::VMT {
			virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL initJni(void* vm, void* context) = 0;
			static constexpr Antilatency::InterfaceContract::InterfaceID ID() {
				return Antilatency::InterfaceContract::InterfaceID{0xbd9f72ed,0xf6e0,0x44dd,{0xb6,0x42,0x57,0x80,0x1f,0x56,0x8c,0xea}};
			}
		private:
			~VMT() = delete;
		};
		IAndroidJni() = default;
		IAndroidJni(std::nullptr_t) {}
		explicit IAndroidJni(VMT* pointer) : Antilatency::InterfaceContract::IInterface(pointer) {}
		template<typename T, typename = typename std::enable_if<std::is_base_of<IAndroidJni, T>::value>::type>
		IAndroidJni& operator = (const T& other) {
		    Antilatency::InterfaceContract::IInterface::operator=(other);
		    return *this;
		}
		template<class Implementer, class ... TArgs>
		static IAndroidJni create(TArgs&&... args) {
		    return *new Implementer(std::forward<TArgs>(args)...);
		}
		void attach(VMT* other) ANTILATENCY_NOEXCEPT {
		    Antilatency::InterfaceContract::IInterface::attach(other);
		}
		VMT* detach() ANTILATENCY_NOEXCEPT {
		    return reinterpret_cast<VMT*>(Antilatency::InterfaceContract::IInterface::detach());
		}
		void initJni(void* vm, void* context) {
			handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->initJni(vm, context));
		}
	};
} //namespace AndroidJniWrapper
namespace Antilatency {
	namespace InterfaceContract {
		namespace Details {
			template<typename Implementer, typename LifeTimeController>
			struct InterfaceRemap<AndroidJniWrapper::IAndroidJni, Implementer, LifeTimeController> : Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController> {
			public:
			    InterfaceRemap() = default;
			    static bool isInterfaceSupported(const Antilatency::InterfaceContract::InterfaceID& id) {
					if (id == AndroidJniWrapper::IAndroidJni::VMT::ID()) {
						return true;
					}
					return Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController>::isInterfaceSupported(id);
				}
			public:
				virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL initJni(void* vm, void* context) {
					try {
						this->_object->initJni(vm, context);
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
