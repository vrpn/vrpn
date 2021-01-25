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
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4946)
#pragma warning(disable: 4265)
#endif
namespace Antilatency {
	namespace Math {
		struct float3 {
			float x;
			float y;
			float z;
		};
	} //namespace Math
} //namespace Antilatency

namespace Antilatency {
	namespace Math {
		struct floatQ {
			float x;
			float y;
			float z;
			float w;
		};
	} //namespace Math
} //namespace Antilatency

namespace Antilatency {
	namespace Math {
		struct floatP3Q {
			Antilatency::Math::float3 position;
			Antilatency::Math::floatQ rotation;
		};
	} //namespace Math
} //namespace Antilatency

namespace Antilatency {
	namespace Math {
		struct float2 {
			float x;
			float y;
		};
	} //namespace Math
} //namespace Antilatency

namespace Antilatency {
	namespace Math {
		struct float2x3 {
			Antilatency::Math::float3 x;
			Antilatency::Math::float3 y;
		};
	} //namespace Math
} //namespace Antilatency

namespace Antilatency {
	namespace Math {
		struct doubleQ {
			double x;
			double y;
			double z;
			double w;
		};
	} //namespace Math
} //namespace Antilatency


#ifdef _MSC_VER
#pragma warning(pop)
#endif
