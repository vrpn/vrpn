// Copyright (c) 2019 ALT LLC
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of source code located below and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//  
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//  
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <cstdint>
#include <array>
#include <atomic>
#include <algorithm>
#include <type_traits>
#include <vector>
#include <string>
#include <cstring>
#include <exception>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4946)
#pragma warning(disable: 4265)
#endif

#if defined(__CYGWIN32__)
#define ANTILATENCY_INTERFACE_CALL __stdcall
#define ANTILATENCY_INTERFACE_EXPORT __declspec(dllexport)
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(WINAPI_FAMILY)
#define ANTILATENCY_INTERFACE_CALL __stdcall
#define ANTILATENCY_INTERFACE_EXPORT __declspec(dllexport)
#elif defined(__MACH__)
#define ANTILATENCY_INTERFACE_CALL
#define ANTILATENCY_INTERFACE_EXPORT
#elif defined(__linux__) || defined(__ANDROID__)
#define ANTILATENCY_INTERFACE_CALL
#define ANTILATENCY_INTERFACE_EXPORT __attribute__ ((visibility ("default")))
#else
#define ANTILATENCY_INTERFACE_CALL
#define ANTILATENCY_INTERFACE_EXPORT
#endif

//#define ANTILATENCY_INTERFACE_CONTRACT_DISABLE_EXCEPTION_DATA
//#define ANTILATENCY_INTERFACE_CONTRACT_DISABLE_LIBRARY

#if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || (defined(_MSC_VER) && defined(_CPPUNWIND)) 
#define ANTILATENCY_INTERFACE_CONTRACT_EXCEPTIONS_ENABLED
#endif

#if defined(ANTILATENCY_INTERFACE_CONTRACT_EXCEPTIONS_ENABLED)
#define ANTILATENCY_NOEXCEPT noexcept
#include <stdexcept>
#else
#define ANTILATENCY_NOEXCEPT
#endif


namespace {
#define  ANTILATENCY_ENUM_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, OP)\
inline constexpr auto operator OP (const ENUM_TYPE_NAME& lhs, const ENUM_TYPE_NAME& rhs) {\
return static_cast<ENUM_TYPE_NAME>(static_cast<UNDERLYING_TYPE>(lhs) OP static_cast<UNDERLYING_TYPE>(rhs)); }\
inline constexpr auto operator OP (const ENUM_TYPE_NAME& lhs, const UNDERLYING_TYPE& rhs) {\
return static_cast<ENUM_TYPE_NAME>(static_cast<UNDERLYING_TYPE>(lhs) OP rhs); }\
inline constexpr auto operator OP (const UNDERLYING_TYPE& lhs, const ENUM_TYPE_NAME& rhs) {\
return static_cast<ENUM_TYPE_NAME>(lhs OP static_cast<UNDERLYING_TYPE>(rhs)); }

#define  ANTILATENCY_ENUM_COMPARISON_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, OP)\
inline constexpr bool operator OP (const ENUM_TYPE_NAME& lhs, const UNDERLYING_TYPE& rhs) {\
return static_cast<UNDERLYING_TYPE>(lhs) OP rhs; }\
inline constexpr bool operator OP (const UNDERLYING_TYPE& lhs, const ENUM_TYPE_NAME& rhs) {\
return lhs OP static_cast<UNDERLYING_TYPE>(rhs); }


#define  ANTILATENCY_COMPOUND_ASSIGNMENT_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, OP)\
inline ENUM_TYPE_NAME& operator OP (ENUM_TYPE_NAME& lhs, const ENUM_TYPE_NAME& rhs) {\
return reinterpret_cast<ENUM_TYPE_NAME&>(reinterpret_cast<UNDERLYING_TYPE&>(lhs) OP static_cast<UNDERLYING_TYPE>(rhs)); }\
inline ENUM_TYPE_NAME& operator OP (ENUM_TYPE_NAME& lhs, const UNDERLYING_TYPE& rhs) {\
return reinterpret_cast<ENUM_TYPE_NAME&>(reinterpret_cast<UNDERLYING_TYPE&>(lhs) OP rhs); }\
inline ENUM_TYPE_NAME& operator OP (UNDERLYING_TYPE& lhs, const ENUM_TYPE_NAME& rhs) {\
return reinterpret_cast<ENUM_TYPE_NAME&>(lhs OP static_cast<UNDERLYING_TYPE>(rhs)); }

#define ANTILATENCY_UNARY_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, OP)\
inline ENUM_TYPE_NAME operator OP (ENUM_TYPE_NAME x) {\
return static_cast<ENUM_TYPE_NAME>(OP static_cast<UNDERLYING_TYPE>(x)); }\

#define ANTILATENCY_MODIFYING_PREFIX_UNARY_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, OP)\
inline ENUM_TYPE_NAME& operator OP (ENUM_TYPE_NAME& x) {\
return reinterpret_cast<ENUM_TYPE_NAME&>(OP reinterpret_cast<UNDERLYING_TYPE&>(x)); }\

#define ANTILATENCY_MODIFYING_POSTFIX_UNARY_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, OP)\
inline ENUM_TYPE_NAME operator OP (ENUM_TYPE_NAME& x, int) {\
return static_cast<ENUM_TYPE_NAME>(reinterpret_cast<UNDERLYING_TYPE&>(x) OP); }\

#define ANTILATENCY_ENUM_INTEGER_BEHAVIOUR_UNSIGNED(ENUM_TYPE_NAME, UNDERLYING_TYPE)\
ANTILATENCY_ENUM_COMPARISON_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, == )\
ANTILATENCY_ENUM_COMPARISON_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, != )\
ANTILATENCY_ENUM_COMPARISON_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, > )\
ANTILATENCY_ENUM_COMPARISON_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, < )\
ANTILATENCY_ENUM_COMPARISON_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, >= )\
ANTILATENCY_ENUM_COMPARISON_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, <= )\
ANTILATENCY_ENUM_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, +)\
ANTILATENCY_ENUM_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, -)\
ANTILATENCY_ENUM_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, *)\
ANTILATENCY_ENUM_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, / )\
ANTILATENCY_ENUM_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, | )\
ANTILATENCY_ENUM_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, &)\
ANTILATENCY_ENUM_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, ^)\
ANTILATENCY_COMPOUND_ASSIGNMENT_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, +=)\
ANTILATENCY_COMPOUND_ASSIGNMENT_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, -=)\
ANTILATENCY_COMPOUND_ASSIGNMENT_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, *=)\
ANTILATENCY_COMPOUND_ASSIGNMENT_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, /=)\
ANTILATENCY_COMPOUND_ASSIGNMENT_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, |=)\
ANTILATENCY_COMPOUND_ASSIGNMENT_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, &=)\
ANTILATENCY_COMPOUND_ASSIGNMENT_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, ^=)\
ANTILATENCY_MODIFYING_PREFIX_UNARY_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, ++)\
ANTILATENCY_MODIFYING_PREFIX_UNARY_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, --)\
ANTILATENCY_MODIFYING_POSTFIX_UNARY_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, ++)\
ANTILATENCY_MODIFYING_POSTFIX_UNARY_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, --)\
ANTILATENCY_UNARY_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, ~)

#define ANTILATENCY_ENUM_INTEGER_BEHAVIOUR_SIGNED(ENUM_TYPE_NAME, UNDERLYING_TYPE)\
ANTILATENCY_ENUM_INTEGER_BEHAVIOUR_UNSIGNED(ENUM_TYPE_NAME, UNDERLYING_TYPE)\
ANTILATENCY_UNARY_OPERATOR(ENUM_TYPE_NAME, UNDERLYING_TYPE, -);
}

namespace Antilatency {
    namespace InterfaceContract {

        static constexpr uint32_t ExceptionCodeSeverityBit = 0x80000000;
        static constexpr uint32_t ExceptionCodeCustomerBit = 0x20000000;
        static constexpr uint32_t ExceptionCodeValueBitsCount = 16;
        static constexpr uint32_t ExceptionCodeFacilityBitsCount = 11;
        static constexpr uint32_t ExceptionCodeFacilityBitsOffset = ExceptionCodeValueBitsCount;

        enum class ExceptionCode : uint32_t {
            Ok = 0,
            NotImplemented = 0x80004001L,
            NoInterface = 0x80004002L,
            ErrorPointer = 0x80004003L,
            UnknownFailure = 0x80004005L,
            OutOfMemory = 0x8007000EL,
            InvalidArg = 0x80070057L,
            Pending = 0x8000000AL,
            AsyncOperationNotStarted = 0x80000019L,
            RpcTimeout = 0x8001011FL,
            RpcDisconnected = 0x80010108L
        };

#ifdef ANTILATENCY_INTERFACE_CONTRACT_EXCEPTIONS_ENABLED
        struct Exception : std::exception {
            Exception() ANTILATENCY_NOEXCEPT :_code(ExceptionCode::UnknownFailure) {}
            Exception(ExceptionCode code) ANTILATENCY_NOEXCEPT : _code(code) {}
            Exception(ExceptionCode code, std::string message) :_code(code), _message(std::move(message)) {}
            Exception(std::string message) :_code(ExceptionCode::UnknownFailure), _message(std::move(message)) {}
            ExceptionCode code() const ANTILATENCY_NOEXCEPT {
                return _code;
            }
            const std::string& message() const ANTILATENCY_NOEXCEPT {
                return _message;
            }

			virtual char const* what() const ANTILATENCY_NOEXCEPT override {
				return _message.c_str();
			}
        private:
            ExceptionCode _code;
            std::string _message;
        };
#endif

        template<typename Implementer>
        inline ExceptionCode handleRemapException(Implementer* implementer) {
#ifdef ANTILATENCY_INTERFACE_CONTRACT_EXCEPTIONS_ENABLED
            try {
                throw;
            }
            catch (const Exception& e) {
                implementer->setMessage(e.message());
                return e.code();
            }
            catch (const std::invalid_argument& ex) {
                implementer->setMessage(ex.what());
                return ExceptionCode::InvalidArg;
            }
            catch (const std::bad_alloc& ex) {
                implementer->setMessage(ex.what());
                return ExceptionCode::OutOfMemory;
            }
            catch (const std::exception& ex) {
                implementer->setMessage(ex.what());
                return ExceptionCode::UnknownFailure;
            }
            catch (...) {
                implementer->setMessage("");
                return ExceptionCode::UnknownFailure;
            }
#else
            return ExceptionCode::UnknownFailure;
#endif
        }

        struct InterfaceID {
            uint32_t Data1;
            uint16_t Data2;
            uint16_t Data3;
            uint8_t Data4[8];

          
            constexpr InterfaceID(uint32_t Data1_, uint16_t Data2_, uint16_t Data3_, std::initializer_list<uint8_t> Data4_) :
                Data1(Data1_), Data2(Data2_), Data3(Data3_), Data4{ Data4_.begin()[0], Data4_.begin()[1] , Data4_.begin()[2] , Data4_.begin()[3] , Data4_.begin()[4] , Data4_.begin()[5] , Data4_.begin()[6] , Data4_.begin()[7] } {

            }

            constexpr InterfaceID(uint32_t Data1_, uint16_t Data2_, uint16_t Data3_, uint8_t(&Data4_)[8]) :
                Data1(Data1_), Data2(Data2_), Data3(Data3_), Data4{ Data4_[0], Data4_[1] , Data4_[2] , Data4_[3] , Data4_[4] , Data4_[5] , Data4_[6] , Data4_[7] } {

            }

            constexpr InterfaceID() :
                Data1(0), Data2(0), Data3(0), Data4{} {
            }

            template<typename TResult>
            static constexpr TResult parseHexDigit(const char digit) {
                if ('0' <= digit && digit <= '9')
                    return static_cast<TResult>(digit - '0');
                else if ('a' <= digit && digit <= 'f')
                    return static_cast<TResult>(10 + digit - 'a');
                else if ('A' <= digit && digit <= 'F')
                    return static_cast<TResult>(10 + digit - 'A');
                else return static_cast<TResult>(-1);
            }

            template<typename TResult>
            static constexpr const char* parseHexNumber(const char* number, TResult& result) {
                int blocksCount = sizeof(TResult) * 2;
                result = 0;
                for (int i = 0; i < blocksCount; ++i) {
                    result |= parseHexDigit<TResult>(number[i]) << static_cast<TResult>(4 * (blocksCount - i - 1));
                }
                return number + blocksCount;
            }

            template<size_t N, bool HaveBraces, bool HaveHyphens>
            static constexpr InterfaceID parseGuidImpl(const char* data) {
                if (HaveBraces) { data += 1; }

                uint32_t n0 = 0;
                data = parseHexNumber(data, n0);

                if (HaveHyphens) { data += 1; }
                uint16_t n1 = 0;
                data = parseHexNumber(data, n1);

                if (HaveHyphens) { data += 1; }
                uint16_t n2 = 0;
                data = parseHexNumber(data, n2);


                uint8_t Data4[8]{};

                if (HaveHyphens) { data += 1; }
                for (int i = 0; i < 8; ++i) {
                    if (i == 2 && HaveHyphens) {
                        data += 1;
                    }
                    data = parseHexNumber(data, Data4[i]);
                }

                return { n0, n1, n2, Data4 };
            }

            template<size_t Length>
            static constexpr InterfaceID parseGuid(const char(&str)[Length]) {
                constexpr size_t N = Length - 1;
                static_assert((N == 32) || (N == 34) || (N == 36) || (N == 38), "Not supported UUID format, can't parse");
                constexpr bool haveBraces = (N == 38) || (N == 34);
                constexpr bool haveHyphens = (N == 38) || (N == 36);
                return parseGuidImpl<N, haveBraces, haveHyphens>(str);
            }

            template<size_t Length>
            constexpr InterfaceID(const char(&str)[Length]) :Data1{ parseGuid(str).Data1 }, Data2{ parseGuid(str).Data2 }, Data3{ parseGuid(str).Data3 }, Data4{
                    parseGuid(str).Data4[0],
                    parseGuid(str).Data4[1],
                    parseGuid(str).Data4[2],
                    parseGuid(str).Data4[3],
                    parseGuid(str).Data4[4],
                    parseGuid(str).Data4[5],
                    parseGuid(str).Data4[6],
                    parseGuid(str).Data4[7],
            } {}

            constexpr bool operator == (const InterfaceID& other) const {
                return (Data1 == other.Data1) && (Data2 == other.Data2) && (Data3 == other.Data3) &&
                    (Data4[0] == other.Data4[0]) &&
                    (Data4[1] == other.Data4[1]) &&
                    (Data4[2] == other.Data4[2]) &&
                    (Data4[3] == other.Data4[3]) &&
                    (Data4[4] == other.Data4[4]) &&
                    (Data4[5] == other.Data4[5]) &&
                    (Data4[6] == other.Data4[6]) &&
                    (Data4[7] == other.Data4[7])
                    ;
            }

            constexpr bool operator != (const InterfaceID& other) const {
                return !(this->operator ==(other));
            }
        };
    }
}

namespace std {
    template<> struct hash<Antilatency::InterfaceContract::InterfaceID> {
        size_t operator()(const Antilatency::InterfaceContract::InterfaceID& guid) const ANTILATENCY_NOEXCEPT {
            const uint64_t* p = reinterpret_cast<const uint64_t*>(&guid);
            hash<uint64_t> h;
            return h(p[0]) ^ h(p[1]);
        }
    };
}


namespace Antilatency {
    namespace InterfaceContract {

        namespace Details {
            namespace PodArrayAccess {
                typedef void* Context;
                typedef void* (ANTILATENCY_INTERFACE_CALL *setSizeCallback)(Context context, uint32_t newSize);

                template<typename T>
                static void* ANTILATENCY_INTERFACE_CALL setVectorSizeDelegate(Context context, uint32_t newSize) {
                    auto& container = *reinterpret_cast<std::vector<T>*>(context);
                    container.resize(newSize);
                    return newSize == 0 ? nullptr : &container[0];
                }

                static void* ANTILATENCY_INTERFACE_CALL setStringSizeDelegate(Context context, uint32_t newSize) {
                    auto& container = *reinterpret_cast<std::string*>(context);
                    container.resize(newSize);
                    //Starting from C++11 std::string's memory is guaranteed to be continuous, so it's valid to get pointer that way
                    return newSize == 0 ? nullptr : &container[0];
                }
            }

            struct ArrayOutMarshaler {
                struct Intermediate {
                    typedef void* Context;
                    typedef void* (ANTILATENCY_INTERFACE_CALL *setSizeCallback)(Context context, uint32_t newSize);

                    Context context;
                    setSizeCallback callback;

                    template<typename T>
                    Intermediate& operator = (const std::vector<T>& source) {
                        void* remoteBuffer = callback(context, static_cast<uint32_t>(source.size()));
                        if (remoteBuffer != nullptr && source.size() != 0) {
                            memcpy(remoteBuffer, &source[0], sizeof(T) * source.size());
                        }
                        return *this;
                    }

                    Intermediate& operator = (const std::string& source) {
                        void* remoteBuffer = callback(context, static_cast<uint32_t>(source.size()));
                        if (remoteBuffer != nullptr && source.size() != 0) {
                            memcpy(remoteBuffer, &source[0], source.size());
                        }
                        return *this;
                    }
                };

                operator Intermediate() const {
                    return intermediate;
                }

                Intermediate intermediate;

                template<typename T>
                static ArrayOutMarshaler create(std::vector<T>& buffer) {
                    ArrayOutMarshaler result;
                    result.intermediate.context = &buffer;
                    result.intermediate.callback = &PodArrayAccess::setVectorSizeDelegate<T>;
                    return result;
                }

                static ArrayOutMarshaler create(std::string& buffer) {
                    ArrayOutMarshaler result;
                    result.intermediate.context = &buffer;
                    result.intermediate.callback = &PodArrayAccess::setStringSizeDelegate;
                    return result;
                }
            };

            struct ArrayInMarshaler {
                struct Intermediate {
                    const void* data;
                    uint32_t size;

                    template<typename T>
                    std::vector<T> toArray() const {
                        std::vector<T> result;
                        result.resize(size);
                        if (size != 0) {
                            memcpy(&result[0], data, sizeof(T) * size);
                        }
                        return result;
                    }

                    std::string toString() const {
                        return std::string(reinterpret_cast<const char*>(data), size);
                    }

                    operator std::string() const {
                        return toString();
                    }

                    template<typename T>
                    operator std::vector<T>() const {
                        return toArray<T>();
                    }
                };


                operator Intermediate() const {
                    return intermediate;
                }

                Intermediate intermediate;

                template<typename T>
                static ArrayInMarshaler create(const std::vector<T>& data)
                {
                    ArrayInMarshaler result;
                    result.intermediate.data = data.data();
                    result.intermediate.size = static_cast<uint32_t>(data.size());
                    return result;
                }

                static ArrayInMarshaler create(const std::string& data)
                {
                    ArrayInMarshaler result;
                    result.intermediate.data = data.data();
                    result.intermediate.size = static_cast<uint32_t>(data.size());
                    return result;
                }
            };

            template<typename TInterface, typename Implementer, typename LifeTimeController = Implementer>
            struct InterfaceRemap {};
        }

        class LongBool {
        public:
            constexpr LongBool() ANTILATENCY_NOEXCEPT = default;
            constexpr LongBool(bool value) ANTILATENCY_NOEXCEPT : _value(toLongBool(value)) {}
            constexpr operator bool() const ANTILATENCY_NOEXCEPT {
                return fromLongBool(_value);
            }
            constexpr bool operator ==(bool other) const ANTILATENCY_NOEXCEPT {
                return fromLongBool(_value) == other;
            }
            constexpr bool operator !=(bool other) const ANTILATENCY_NOEXCEPT {
                return fromLongBool(_value) != other;
            }
            LongBool& operator =(bool other) ANTILATENCY_NOEXCEPT {
                _value = toLongBool(other);
                return *this;
            }
        private:
            static constexpr uint32_t toLongBool(bool value) ANTILATENCY_NOEXCEPT {
                return value ? TrueValue : FalseValue;
            }
            static constexpr bool fromLongBool(uint32_t value) ANTILATENCY_NOEXCEPT {
                return value != FalseValue;
            }
            static constexpr uint32_t TrueValue = 0xffffffffu;
            static constexpr uint32_t FalseValue = 0x0u;
            uint32_t _value = FalseValue;
        };

        struct IUnsafe {
            struct VMT {
                virtual ExceptionCode ANTILATENCY_INTERFACE_CALL QueryInterface(const InterfaceID& riid, void** ppvObject) = 0;
                static constexpr InterfaceID ID() {
                    return InterfaceID{ 0xffffffff, 0xffff, 0xffff, { 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64 } };
                }
            private:
                ~VMT() = delete;
            };

            //From nullptr
            IUnsafe() = default;
            IUnsafe(std::nullptr_t) {}
            explicit IUnsafe(VMT* ptr) :_object(ptr) {

            }

            void attach(VMT* other) ANTILATENCY_NOEXCEPT {
                _object = other;
            }

            VMT* detach() ANTILATENCY_NOEXCEPT {
                VMT* temp = _object;
                _object = nullptr;
                return temp;
            }

            bool operator == (const IUnsafe& other) const {
                return _object == other._object;
            }

            bool operator != (const IUnsafe& other) const {
                return _object != other._object;
            }
            bool operator == (std::nullptr_t) const {
                return _object == nullptr;
            }

            bool operator != (std::nullptr_t) const {
                return _object != nullptr;
            }

            VMT* operator*() {
                return reinterpret_cast<VMT*>(_object);
            }

            const VMT* operator*() const {
                return reinterpret_cast<VMT*>(_object);
            }

            operator bool() const {
                return (_object != nullptr);
            }

            template<typename TInterface, typename = typename std::enable_if<std::is_base_of<IUnsafe, TInterface>::value>::type>
            TInterface queryInterface() {
                if (_object == nullptr) {
                    return TInterface{};
                }

                TInterface result{};
                _object->QueryInterface(TInterface::VMT::ID(), reinterpret_cast<void**>(&result));
                return result;
            }

        protected:
            VMT* _object = nullptr;

            void* operator new(std::size_t size) = delete;

            template<typename T = float>
            void handleExceptionCode(ExceptionCode code);
        };

        struct IInterface;

        namespace Details {

            template<typename Remap>
            static constexpr bool isSafeInterfaceRemap() {
                using Implementer = typename Remap::TImplementer;
                using LifeTimeController = typename Remap::TLifeTimeController;
                using SafeRemap = Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IInterface, Implementer, LifeTimeController>;
                return std::is_base_of<SafeRemap, Remap>::value;
            }

            struct IRemapContainer {
				virtual ~IRemapContainer() = default;
                IRemapContainer* next;
                virtual void getInterfacePointer(const Antilatency::InterfaceContract::InterfaceID& interfaceId, void** result, bool& isSafeInterfac) = 0;
            };

            struct RemapContainerHead : IRemapContainer {
                RemapContainerHead() {
                    next = this;
                }
                void getInterfacePointer(const Antilatency::InterfaceContract::InterfaceID&, void** result, bool&) override final {
                    *result = nullptr;
                }
            };

            template<typename Remap>
            struct RemapContainer : IRemapContainer {
                Remap remap;
                using Implementer = typename Remap::TImplementer;
                using Controller = typename Remap::TLifeTimeController;
                RemapContainer() = delete;

                RemapContainer(Implementer* implementer) {
                    remap._object = implementer;
                    static_cast<Controller*>(implementer)->AddRemap(this);
                }

                void getInterfacePointer(const Antilatency::InterfaceContract::InterfaceID& interfaceId, void** result, bool& isSafeInterface) override final {
                    //Check if remap supports GUID
                    if (Remap::isInterfaceSupported(interfaceId)) {
                        isSafeInterface = Antilatency::InterfaceContract::Details::isSafeInterfaceRemap<Remap>();
                        *result = &remap;
                    }
                    else {
                        *result = nullptr;
                    }
                }
            };
        }

        namespace Details {

            template<typename Implementer, typename LifeTimeController>
            struct InterfaceRemap<IUnsafe, Implementer, LifeTimeController> {
            public:
                template<typename Remap>
                friend struct RemapContainer;
                using TImplementer = Implementer;
                using TLifeTimeController = LifeTimeController;
                InterfaceRemap() = default;
                static bool isInterfaceSupported(const Antilatency::InterfaceContract::InterfaceID& id) {
                    if (id == IUnsafe::VMT::ID()) {
                        return true;
                    }
                    return false;
                }
                virtual ExceptionCode ANTILATENCY_INTERFACE_CALL QueryInterface(const InterfaceID& riid, void** ppvObject) {
                    return static_cast<LifeTimeController*>(this->_object)->QueryInterface(riid, ppvObject);
                }
            protected:
                ~InterfaceRemap() = default;
                Implementer* _object = nullptr;
            };

            template<typename Remap>
            struct RemapPatch final : Remap {
                using Remap::Remap;
                ~RemapPatch() = default;
            };
        }

        struct IInterface : IUnsafe {
            struct VMT : IUnsafe::VMT {
                virtual uint32_t ANTILATENCY_INTERFACE_CALL AddRef() = 0;
                virtual uint32_t ANTILATENCY_INTERFACE_CALL Release() = 0;

                static constexpr InterfaceID ID() {
                    return InterfaceID{ 0x00000000, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
                }
            private:
                ~VMT() = delete;
            };

            //From nullptr
            IInterface() = default;
            IInterface(std::nullptr_t) {}

            template<class Implementer, class ... TArgs>
            static IInterface create(TArgs&& ... args) {
                return *new Implementer(std::forward<TArgs>(args)...);
            }

            explicit IInterface(VMT* ptr) : IUnsafe(ptr) {
                internalAddRef();
            }

            IInterface(const IInterface& other) : IUnsafe(other._object) {
                internalAddRef();
            }

            void attach(VMT* other) ANTILATENCY_NOEXCEPT {
                internalRelease();
                Antilatency::InterfaceContract::IUnsafe::attach(other);
            }
            VMT* detach() ANTILATENCY_NOEXCEPT {
                return reinterpret_cast<VMT*>(Antilatency::InterfaceContract::IUnsafe::detach());
            }

            IInterface operator = (const IInterface& other) {
                internalCopy(reinterpret_cast<VMT*>(other._object));
                return *this;
            }

            IInterface operator = (std::nullptr_t) {
                internalRelease();
                return *this;
            }

            bool operator == (const IInterface& other) const {
                return _object == other._object;
            }

            bool operator != (const IInterface& other) const {
                return _object != other._object;
            }
            bool operator == (std::nullptr_t) const {
                return _object == nullptr;
            }

            bool operator != (std::nullptr_t) const {
                return _object != nullptr;
            }

            VMT* operator*() {
                return reinterpret_cast<VMT*>(_object);
            }

            const VMT* operator*() const {
                return reinterpret_cast<VMT*>(_object);
            }

            operator bool() const {
                return (_object != nullptr);
            }

            ~IInterface() {
                internalRelease();
            }

        private:
            void internalAddRef() const ANTILATENCY_NOEXCEPT {
                if (_object) {
                    reinterpret_cast<VMT*>(_object)->AddRef();
                }
            }

            void internalRelease() ANTILATENCY_NOEXCEPT {
                VMT* temp = reinterpret_cast<VMT*>(_object);
                if (temp) {
                    _object = nullptr;
                    temp->Release();
                }
            }

            void internalCopy(VMT* other) ANTILATENCY_NOEXCEPT {
                if (_object != other) {
                    internalRelease();
                    _object = other;
                    internalAddRef();
                }
            }
        };

        //TODO: std::is_base_of<IUnsafe, T> ?
        template<typename T, bool Yes = std::is_base_of<IInterface, T>::value>
        struct IsInterface : std::true_type {};

        template<typename T>
        struct IsInterface<T, false> : std::false_type {};

        namespace Details {
            template<typename Implementer, typename LifeTimeController>
            struct InterfaceRemap<IInterface, Implementer, LifeTimeController> : InterfaceRemap<IUnsafe, Implementer, LifeTimeController> {
            public:
                InterfaceRemap() = default;
                static bool isInterfaceSupported(const Antilatency::InterfaceContract::InterfaceID& id) {
                    if (id == IInterface::VMT::ID()) {
                        return true;
                    }
                    return InterfaceRemap<IUnsafe, Implementer, LifeTimeController>::isInterfaceSupported(id);
                }
                virtual uint32_t ANTILATENCY_INTERFACE_CALL AddRef() {
                    return static_cast<LifeTimeController*>(this->_object)->AddRef();
                }
                virtual uint32_t ANTILATENCY_INTERFACE_CALL Release() {
                    return static_cast<LifeTimeController*>(this->_object)->Release();
                }
            };
        }

#ifndef ANTILATENCY_INTERFACE_CONTRACT_DISABLE_EXCEPTION_DATA
        struct IExceptionData : Antilatency::InterfaceContract::IUnsafe {
            struct VMT : Antilatency::InterfaceContract::IUnsafe::VMT {
                virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getMessage(Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate message) = 0;
                virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL setMessage(Antilatency::InterfaceContract::Details::ArrayInMarshaler::Intermediate message) = 0;
                static constexpr Antilatency::InterfaceContract::InterfaceID ID() {
                    return Antilatency::InterfaceContract::InterfaceID{ 0x97122ff5, 0xceaa, 0x4c40, { 0x96, 0x27, 0x12, 0xaa, 0xb7, 0x4a, 0x5d, 0xaf } };
                }
            private:
                ~VMT() = delete;
            };
            IExceptionData() = default;
            IExceptionData(std::nullptr_t) {}
            explicit IExceptionData(VMT* pointer) : Antilatency::InterfaceContract::IUnsafe(pointer) {}
            template<typename T, typename = typename std::enable_if<std::is_base_of<IExceptionData, T>::value>::type>
            IExceptionData& operator = (const T& other) {
                Antilatency::InterfaceContract::IUnsafe::operator=(other);
                return *this;
            }
            template<class Implementer, class ... TArgs>
            static IExceptionData create(TArgs&& ... args) {
                return *new Implementer(std::forward<TArgs>(args)...);
            }
            void attach(VMT* other) ANTILATENCY_NOEXCEPT {
                Antilatency::InterfaceContract::IUnsafe::attach(other);
            }
            VMT* detach() ANTILATENCY_NOEXCEPT {
                return reinterpret_cast<VMT*>(Antilatency::InterfaceContract::IUnsafe::detach());
            }
            void setMessage(const std::string& message) {
                auto valueMarshaler = Antilatency::InterfaceContract::Details::ArrayInMarshaler::create(message);
                handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->setMessage(valueMarshaler));
            }
            std::string getMessage() {
                std::string result;
                auto resultMarshaler = Antilatency::InterfaceContract::Details::ArrayOutMarshaler::create(result);
                handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->getMessage(resultMarshaler));
                return result;
            }
        };
#endif

        template<typename T>
        inline void IUnsafe::handleExceptionCode(ExceptionCode code) {
#ifdef ANTILATENCY_INTERFACE_CONTRACT_EXCEPTIONS_ENABLED
#ifdef ANTILATENCY_INTERFACE_CONTRACT_DISABLE_EXCEPTION_DATA
            if (code != ExceptionCode::Ok) {
                throw Antilatency::InterfaceContract::Exception(code);
            }
#else
            if (code != ExceptionCode::Ok) {
                auto exceptionData = this->template queryInterface<Antilatency::InterfaceContract::IExceptionData>();
                throw Antilatency::InterfaceContract::Exception(code, exceptionData != nullptr ? exceptionData.getMessage() : "");
            }
#endif
#else
            static_cast<void>(code);
#endif
        }

#ifdef ANTILATENCY_INTERFACE_CONTRACT_EXCEPTIONS_ENABLED
        namespace Details {
            template<typename Implementer, typename LifeTimeController>
            struct InterfaceRemap<IExceptionData, Implementer, LifeTimeController> : Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IUnsafe, Implementer, LifeTimeController> {
                InterfaceRemap() = default;
                static bool isInterfaceSupported(const Antilatency::InterfaceContract::InterfaceID& id) {
                    if (id == IExceptionData::VMT::ID()) {
                        return true;
                    }
                    return Antilatency::InterfaceContract::Details::InterfaceRemap<Antilatency::InterfaceContract::IUnsafe, Implementer, LifeTimeController>::isInterfaceSupported(id);
                }
                virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getMessage(Antilatency::InterfaceContract::Details::ArrayOutMarshaler::Intermediate message) {
#ifdef __cpp_exceptions
                    try {
#endif
                        message = this->_object->getMessage();
#ifdef __cpp_exceptions
                    }
                    catch (...) {
                        return Antilatency::InterfaceContract::ExceptionCode::UnknownFailure;
                    }
#endif
                    return Antilatency::InterfaceContract::ExceptionCode::Ok;
                }
                virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL setMessage(Antilatency::InterfaceContract::Details::ArrayInMarshaler::Intermediate message) {
#ifdef __cpp_exceptions
                    try {
#endif
                        this->_object->setMessage(message);
#ifdef __cpp_exceptions
                    }
                    catch (...) {
                        return Antilatency::InterfaceContract::ExceptionCode::UnknownFailure;
                    }
#endif
                    return Antilatency::InterfaceContract::ExceptionCode::Ok;
                }
            };
        }
#endif

#define ANTILATENCY_INTERFACE_CONTRACT_CONCAT2(a, b) a ## b
#define ANTILATENCY_INTERFACE_CONTRACT_CONCAT(a, b) ANTILATENCY_INTERFACE_CONTRACT_CONCAT2(a, b)
#define IMPLEMENT_INTERFACE(ImplementerType, InterfaceName) \
    private: Antilatency::InterfaceContract::Details::RemapContainer<Antilatency::InterfaceContract::Details::RemapPatch<Antilatency::InterfaceContract::Details::InterfaceRemap<InterfaceName,ImplementerType> > > ANTILATENCY_INTERFACE_CONTRACT_CONCAT(__InterfaceRemap_, __LINE__) = { this }; public: operator InterfaceName() { return InterfaceName(reinterpret_cast<typename InterfaceName::VMT*>(&ANTILATENCY_INTERFACE_CONTRACT_CONCAT(__InterfaceRemap_, __LINE__).remap));} \
    private:

#define IMPLEMENT_INTERFACE_EX(ImplementerType, InterfaceName, LifeTimeControllerType) \
    private: Antilatency::InterfaceContract::Details::RemapContainer<Antilatency::InterfaceContract::Details::RemapPatch<Antilatency::InterfaceContract::Details::InterfaceRemap<InterfaceName,ImplementerType,LifeTimeControllerType> > > ANTILATENCY_INTERFACE_CONTRACT_CONCAT(__InterfaceRemap_, __LINE__) = { this }; public: operator InterfaceName() { return InterfaceName(reinterpret_cast<typename InterfaceName::VMT*>(&ANTILATENCY_INTERFACE_CONTRACT_CONCAT(__InterfaceRemap_, __LINE__).remap));} \
    private:

#define ANTILATENCY_INTERFACE_DEFINE_DLL_ENTRY_TEMPLATE(AntilatencyExport, AntilatencyAlternateExport) \
    extern "C" AntilatencyExport Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL getLibraryInterface(Antilatency::InterfaceContract::ILibraryUnloader::VMT* unloaderVmt, Antilatency::InterfaceContract::IInterface& result){ \
        AntilatencyAlternateExport\
        if(unloaderVmt != nullptr){ \
            Antilatency::InterfaceContract::ILibraryUnloader libraryUnloader; \
            libraryUnloader.attach(unloaderVmt); \
            Antilatency::InterfaceContract::Details::LibraryProvider::instance().addUnloader(libraryUnloader); \
        } \
        result = ILibrary::create<Library>(); \
        return Antilatency::InterfaceContract::ExceptionCode::Ok; \
    }

#if defined(_MSC_VER) && !defined(_WIN64)
    #define ANTILATENCY_MSVC_X86_EXPORT __pragma(comment(linker, "/EXPORT:getLibraryInterface=" __FUNCDNAME__));
    #define ANTILATENCY_INTERFACE_DEFINE_DLL_ENTRY ANTILATENCY_INTERFACE_DEFINE_DLL_ENTRY_TEMPLATE(, ANTILATENCY_MSVC_X86_EXPORT)
#else
    #define ANTILATENCY_INTERFACE_DEFINE_DLL_ENTRY ANTILATENCY_INTERFACE_DEFINE_DLL_ENTRY_TEMPLATE(ANTILATENCY_INTERFACE_EXPORT, )
#endif

        struct ILibraryUnloader : Antilatency::InterfaceContract::IInterface {
            struct VMT : public IInterface::VMT {
                virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL unloadLibrary() = 0;

                static constexpr Antilatency::InterfaceContract::InterfaceID ID() {
                    return Antilatency::InterfaceContract::InterfaceID{ 0x9154edf5, 0x09e7, 0x45a7, { 0x99, 0xd9, 0x63, 0x80, 0x65, 0x9f, 0xa6, 0xf2 } };
                }
            private:
                ~VMT() = delete;
            };

            //Begin default methods
            ILibraryUnloader() = default;
            ILibraryUnloader(VMT* pointer) : IInterface(pointer) {}

            template<typename T, typename = typename std::enable_if<std::is_base_of<ILibraryUnloader, T>::value>::type>
            ILibraryUnloader& operator = (const T& other) {
                IInterface::operator=(other);
                return *this;
            }

            template<class Implementer, class ... TArgs>
            static ILibraryUnloader create(TArgs&& ... args) {
                return *new Implementer(std::forward<TArgs>(args)...);
            }
            void attach(VMT* other) ANTILATENCY_NOEXCEPT {
                Antilatency::InterfaceContract::IInterface::attach(other);
            }
            VMT* detach() ANTILATENCY_NOEXCEPT {
                return reinterpret_cast<VMT*>(Antilatency::InterfaceContract::IInterface::detach());
            }//End default methods


            //Begin interface method wrappers
            void unloadLibrary() {
                handleExceptionCode((reinterpret_cast<VMT*>(this->_object))->unloadLibrary());
            }
        };

        namespace Details {
            template<typename Implementer, typename LifeTimeController>
            struct InterfaceRemap<ILibraryUnloader, Implementer, LifeTimeController> : Antilatency::InterfaceContract::Details::InterfaceRemap<IInterface, Implementer, LifeTimeController> {
                InterfaceRemap() = default;
                static bool isInterfaceSupported(const Antilatency::InterfaceContract::InterfaceID& id) {
                    if (id == ILibraryUnloader::VMT::ID()) {
                        return true;
                    }
                    return Antilatency::InterfaceContract::Details::InterfaceRemap<IInterface, Implementer, LifeTimeController>::isInterfaceSupported(id);
                }
                virtual Antilatency::InterfaceContract::ExceptionCode ANTILATENCY_INTERFACE_CALL unloadLibrary() {
                    (*reinterpret_cast<Implementer*>(this->_object)).unloadLibrary();
                    return Antilatency::InterfaceContract::ExceptionCode::Ok;
                }
            };
        }

        class InterfacedObject;
        class LifetimeControllerBase;

#ifndef ANTILATENCY_INTERFACE_CONTRACT_DISABLE_LIBRARY
        namespace Details {
            struct LibraryProvider {
            private:
                friend class ::Antilatency::InterfaceContract::InterfacedObject;
                friend class ::Antilatency::InterfaceContract::LifetimeControllerBase;

                std::vector<ILibraryUnloader> _unloaders;
                std::atomic<size_t> _libraryRefCount = {};


                void addRef() {
                    _libraryRefCount++;
                }

                void release() {
                    if (1 == _libraryRefCount.fetch_sub(1)) {
                        for (auto it = _unloaders.rbegin(); it != _unloaders.rend(); ++it) {
                            (*it).unloadLibrary();
                        }
                        _unloaders.clear();
                    }
                }

            public:
                void addUnloader(ILibraryUnloader unloader) {
                    _unloaders.push_back(unloader);
                }
                static LibraryProvider& instance() {
                    static LibraryProvider _instance;
                    return _instance;
                }
                size_t getLiveObjectsCount() {
                    return _libraryRefCount.load();
                }
            };
        }
#endif

#if 0
        struct IWeakReference : IInterface {
            struct VMT : IInterface::VMT {
                static constexpr InterfaceID ID() {
                    return InterfaceID{ 0xda9a1261, 0x2e47, 0x4cde, { 0x8a, 0x0c, 0x80, 0x52, 0x7a, 0x30, 0xf9, 0x6b } };
                }
            private:
                ~VMT() = delete;
            };

            IWeakReference() = default;
            IWeakReference(std::nullptr_t) {}
            explicit IWeakReference(VMT* pointer) : IInterface(pointer) {}
            template<typename T, typename = typename std::enable_if<std::is_base_of<IWeakReference, T>::value>::type>
            IWeakReference& operator = (const T& other) {
                IInterface::operator=(other);
                return *this;
            }
            void attach(VMT* other) ANTILATENCY_NOEXCEPT {
                IInterface::attach(other);
            }
            VMT* detach() ANTILATENCY_NOEXCEPT {
                return reinterpret_cast<VMT*>(Antilatency::InterfaceContract::IInterface::detach());
            }
        };

        struct WeakReferenceImpl final {
            enum class PointerState : int {
                Valid,
                Invalid,
                LockedByWeak
            };

            std::atomic<PointerState> _targetState = {};
            std::atomic<size_t> _weakDepth = {};
            std::atomic<IInterface::VMT*> _target;

            virtual ExceptionCode ANTILATENCY_INTERFACE_CALL QueryInterface(const InterfaceID& riid, void** ppvObject) {

                PointerState desiredOldState = PointerState::Valid;

                //Retry WeakLock
                while (!_targetState.compare_exchange_strong(desiredOldState, PointerState::LockedByWeak)) {
                    if (desiredOldState == PointerState::Invalid) { //Object has been destroyed already
                        ppvObject = nullptr;
                        return ExceptionCode::Ok;
                    }
                    else {
                        //Another thread won (in getting WeakReference) -> wait for that thread to release WeakLock (spin-waiting because i don't want to use mutexes)
                        desiredOldState = PointerState::Valid;
                    }
                }

                auto ptr = _target.load();
                auto result = ptr->QueryInterface(riid, ppvObject);

                //This call switched state to "LockedByWeak" so unlock it now
                _targetState = PointerState::Valid;
                return result;
            }

            virtual uint32_t ANTILATENCY_INTERFACE_CALL AddRef() {
                auto oldRefCount = _refCount.fetch_add(1);
                return oldRefCount + 1;
            }
            virtual uint32_t ANTILATENCY_INTERFACE_CALL Release() {
                //TODO: WeakLock. If old state was invalid &&weak refcount == 0 -> delete this object

                unsigned long ulRefCount = _refCount.fetch_sub(1);
                if (ulRefCount == 1) {
                    delete this;
                }
                return ulRefCount - 1;
            }
        private:
            std::atomic<unsigned long> _refCount{};
        };
#endif



        class LifetimeControllerBase {
        public:
            template<typename Implementer>
            friend ExceptionCode Antilatency::InterfaceContract::handleRemapException(Implementer* implementer);

            template<typename TInterface, typename TImplementer, typename LifeTimeController>
            friend struct Antilatency::InterfaceContract::Details::InterfaceRemap;
            template<typename T>
            friend struct Antilatency::InterfaceContract::Details::RemapContainer;

            LifetimeControllerBase() {
#ifndef ANTILATENCY_INTERFACE_CONTRACT_DISABLE_LIBRARY
                Details::LibraryProvider::instance().addRef();
#endif
            }

            virtual ~LifetimeControllerBase() {
#ifndef ANTILATENCY_INTERFACE_CONTRACT_DISABLE_LIBRARY
                Details::LibraryProvider::instance().release();
#endif
            }
        protected:
            uint32_t ANTILATENCY_INTERFACE_CALL GlobalAddRef() {
                auto oldRefCount = _globalRefCount.fetch_add(1);
                return oldRefCount + 1;
            }
            uint32_t ANTILATENCY_INTERFACE_CALL GlobalRelease() {
                unsigned long ulRefCount = _globalRefCount.fetch_sub(1);
                if (ulRefCount == 1) {
                    destroy();
                }
                return ulRefCount - 1;
            }
            virtual void destroy() {
                delete this;
            }
        private:
            std::atomic<unsigned long> _globalRefCount{};

#ifdef ANTILATENCY_INTERFACE_CONTRACT_EXCEPTIONS_ENABLED
#ifndef ANTILATENCY_INTERFACE_CONTRACT_DISABLE_EXCEPTION_DATA
            static std::string& getExceptionContext() {
                thread_local std::string context;
                return context;
            }
            void setMessage(const std::string& message) {
                getExceptionContext() = message;
            }
            std::string getMessage() {
                return getExceptionContext();
            }
#endif
#endif
        };

        template<int ID>
        class LifeTimeController : public virtual LifetimeControllerBase {
        public:
            template<typename TInterface, typename TImplementer, typename LifeTimeController>
            friend struct Antilatency::InterfaceContract::Details::InterfaceRemap;

            template<typename TObject, int LifetimeID>
            friend struct ObjectPtr;

            template<typename TInterface, typename TImplementer, typename LifeTimeController>
            friend struct Antilatency::InterfaceContract::Details::InterfaceRemap;

            template<typename T>
            friend struct Antilatency::InterfaceContract::Details::RemapContainer;

            ExceptionCode ANTILATENCY_INTERFACE_CALL QueryInterface(const InterfaceID& riid, void** ppvObject) {
                if (ppvObject == nullptr) {
                    return ExceptionCode::ErrorPointer;
                }

                bool isSafeInterface;
                FindInterface(riid, ppvObject, isSafeInterface);

                if (*ppvObject == nullptr) {
                    return ExceptionCode::NoInterface;
                }
                else {
                    if (isSafeInterface) {
                        auto obj = (reinterpret_cast<IInterface::VMT*>(*ppvObject));
                        obj->AddRef();
                    }
                    return ExceptionCode::Ok;
                }
            }
        protected:
            uint32_t ANTILATENCY_INTERFACE_CALL AddRef() {
                auto oldRefCount = partialRefCount.fetch_add(1);
                LifetimeControllerBase::GlobalAddRef();
                return oldRefCount + 1;
            }

            uint32_t ANTILATENCY_INTERFACE_CALL Release() {
                unsigned long ulRefCount = partialRefCount.fetch_sub(1);
                if (ulRefCount == 1) {
                    partialDestroy(ID);
                }
                LifetimeControllerBase::GlobalRelease();
                return ulRefCount - 1;
            }

            virtual void partialDestroy(int /*lifetimeId*/) {}
        private: 
            Details::RemapContainerHead _interfaceChain;
            std::atomic<unsigned long> partialRefCount{};

            void FindInterface(const InterfaceID& id, void** result, bool& isSafeInterface) {
                Details::IRemapContainer* current = _interfaceChain.next;
                while (current != &_interfaceChain) {
                    current->getInterfacePointer(id, result, isSafeInterface);
                    if (*result != nullptr) {
                        return;
                    }
                    current = current->next;
                }
                *result = nullptr;
            }

            void AddRemap(Details::IRemapContainer* record) {
                record->next = _interfaceChain.next;
                _interfaceChain.next = record;
            }

            IMPLEMENT_INTERFACE(LifeTimeController<ID>, Antilatency::InterfaceContract::IInterface)
#ifdef ANTILATENCY_INTERFACE_CONTRACT_EXCEPTIONS_ENABLED
#ifndef ANTILATENCY_INTERFACE_CONTRACT_DISABLE_EXCEPTION_DATA
            IMPLEMENT_INTERFACE(LifeTimeController<ID>, Antilatency::InterfaceContract::IExceptionData)
#endif
#endif
        };

        class InterfacedObject : public LifeTimeController<0> {
            using Base = LifeTimeController<0>;
            using Base::Base;
        };


#ifndef ANTILATENCY_INTERFACE_CONTRACT_DISABLE_LIBRARY
        template<typename LibraryInterface>
        class Library : public InterfacedObject {
            IMPLEMENT_INTERFACE(Antilatency::InterfaceContract::Library<LibraryInterface>, Antilatency::InterfaceContract::ILibraryUnloader)
        public:
            typedef ExceptionCode(ANTILATENCY_INTERFACE_CALL *LibraryEntryPoint)(Antilatency::InterfaceContract::ILibraryUnloader::VMT* unloader, Antilatency::InterfaceContract::IInterface& result);
            LibraryInterface getLibraryInterface() {
                Antilatency::InterfaceContract::IInterface result;
                Antilatency::InterfaceContract::ILibraryUnloader unloader = *this;
                auto entryPoint = getEntryPoint();
                if (entryPoint == nullptr) {
                    return {};
                }
                entryPoint(unloader.detach(), result);
                return result.template queryInterface<LibraryInterface>();
            }
            virtual void unloadLibrary() {
                //unloadLibraryImpl();
            }
        protected:
            Library() {}
			virtual void unloadLibraryImpl() = 0;
            virtual LibraryEntryPoint getEntryPoint() = 0;
        };
#endif

        template<typename TObject, int LifetimeID = 0>
        struct ObjectPtr {

            template<typename TOtherObject, int OtherLifetimeID>
            friend struct ObjectPtr;

            ObjectPtr() : _object(nullptr) {}

            //Copy construct
            explicit ObjectPtr(TObject* ptr) : _object(ptr) {
                internalAddRef();
            }
            ObjectPtr(const ObjectPtr& other) : _object(other._object) {
                internalAddRef();
            }
            template<typename U, typename = std::enable_if_t<std::is_base_of<TObject, U>::value>>
            ObjectPtr(const ObjectPtr<U>& other) : _object(static_cast<TObject*>(other.get())) {
                internalAddRef();
            }

            //Move construct
            ObjectPtr(ObjectPtr&& other) : _object(other.get()) {
                internalAddRef();
                other.internalRelease();
            }

            template<typename U, typename = std::enable_if_t<std::is_base_of<TObject, U>::value>>
            ObjectPtr(ObjectPtr<U>&& other) : _object(static_cast<TObject*>(other.get())) {
                internalAddRef();
                other.internalRelease();
            }

            //Copy assign
            ObjectPtr& operator = (std::nullptr_t) {
                internalRelease();
                return *this;
            }
            ObjectPtr& operator = (const ObjectPtr& other) {
                internalCopy(other.get());
                return *this;
            }
            template<typename U, typename = std::enable_if_t<std::is_base_of<TObject, U>::value>>
            ObjectPtr& operator = (const ObjectPtr<U>& other) {
                internalCopy(static_cast<TObject*>(other.get()));
                return *this;
            }

            //Move assign
            ObjectPtr& operator = (ObjectPtr&& other) ANTILATENCY_NOEXCEPT {
                if (_object != other._object) {
                    internalRelease();
                    _object = other.get();
                    internalAddRef();
                    other.internalRelease();
                }
                return *this;
            }


            /*template<typename U, typename I, typename = std::enable_if_t<std::is_base_of<TObject, U>::value>>
            ObjectPtr& operator = (ObjectPtr<U, I>&& other) ANTILATENCY_NOEXCEPT {
                internalRelease();
                _object = static_cast<TObject*>(other.detach());
                return *this;
            }*/


            ~ObjectPtr() = default;

            template<typename U, int I, typename = std::enable_if_t<std::is_base_of<U, TObject>::value>>
            explicit operator ObjectPtr<U, I>() {
                ObjectPtr<U, I> result(static_cast<U*>(get()));
                return result;
            }

            template<typename... TArgs>
            static ObjectPtr create(TArgs&& ... args) {
                return ObjectPtr(new TObject(std::forward<TArgs>(args)...));
            }

            TObject* operator ->() ANTILATENCY_NOEXCEPT {
                return _object;
            }

            const TObject* operator ->() const ANTILATENCY_NOEXCEPT {
                return _object;
            }

            bool operator == (const ObjectPtr& other) const ANTILATENCY_NOEXCEPT {
                return _object == other._object;
            }

            bool operator == (std::nullptr_t) const ANTILATENCY_NOEXCEPT {
                return _object == nullptr;
            }

            bool operator != (const ObjectPtr& other) const ANTILATENCY_NOEXCEPT {
                return _object != other._object;
            }

            bool operator != (std::nullptr_t) const ANTILATENCY_NOEXCEPT {
                return _object != nullptr;
            }

            operator bool() const ANTILATENCY_NOEXCEPT {
                return _object != nullptr;
            }

            TObject* get() const ANTILATENCY_NOEXCEPT {
                return _object;
            }

            template<typename TargetInterface, typename = typename std::enable_if<std::is_base_of<IUnsafe, TargetInterface>::value>::type>
            TargetInterface queryInterface() {
                if (_object == nullptr) {
                    return nullptr;
                }
                return _interface.template queryInterface<TargetInterface>();
            }

            template<typename TargetInterface, int OtherLifetimeID = 0, typename = typename std::enable_if<std::is_base_of<IUnsafe, TargetInterface>::value && std::is_base_of<Antilatency::InterfaceContract::LifeTimeController<OtherLifetimeID>, TObject>::value>::type>
            TargetInterface queryLifetimeInterface() {
                if (_object == nullptr) {
                    return nullptr;
                }
                using VMT = typename TargetInterface::VMT;
                TargetInterface result{};
  
                auto err = static_cast<Antilatency::InterfaceContract::LifeTimeController<OtherLifetimeID>*>(_object)->QueryInterface(VMT::ID(), reinterpret_cast<void**>(&result));
                if (err != Antilatency::InterfaceContract::ExceptionCode::Ok) {
                    return nullptr;
                }
                return result;
            }

            /*TInt* detach(){
                auto tmp = _object;
                _object = nullptr;
                if (tmp != nullptr) {
                    _interface.detach();
                }
                _interface = nullptr;
                return tmp;
            }*/
        private:
            void internalAddRef() ANTILATENCY_NOEXCEPT {
                if (_object) {
                    _interface = *static_cast<LifeTimeController<LifetimeID>*>(_object);
                }
            }

            void internalRelease() ANTILATENCY_NOEXCEPT {
                TObject* temp = _object;
                if (temp) {
                    _object = nullptr;
                    _interface = nullptr;
                }
            }

            void internalCopy(TObject* other) ANTILATENCY_NOEXCEPT {
                if (_object != other) {
                    internalRelease();
                    _object = other;
                    internalAddRef();
                }
            }
            TObject* _object;
            Antilatency::InterfaceContract::IInterface _interface;
        };
    }
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif
