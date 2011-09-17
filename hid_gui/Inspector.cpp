/**
	@file
	@brief Implementation

	@date 2011

	@author
	Ryan Pavlik
	<rpavlik@iastate.edu> and <abiryan@ryand.net>
	http://academic.cleardefinition.com/
	Iowa State University Virtual Reality Applications Center
	Human-Computer Interaction Graduate Program
*/

//          Copyright Iowa State University 2011.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// Internal Includes
#include "Inspector.h"
#include "vrpn_Shared.h"

// Library/third-party includes
// - none

// Standard includes
#include <stdexcept>
#include <iostream>
//#include <stdint.h>

template<typename T>
T getFromByteArray(QByteArray const& input) {
	union {
		char bytes[sizeof(T)];
		T value;
	};
	for (int i = 0; i < sizeof(T); ++i) {
		bytes[i] = input.at(i);
	}
	return value;
}

Inspector::Inspector(std::size_t first_index, std::size_t length, bool signedVal, bool bigEndian, QObject * parent)
	: QObject(parent)
	, _first(first_index)
	, _length(length)
	, _signed(signedVal)
	, _bigEndian(bigEndian) {
	switch (_length) {
		case 1:
		case 2:
		case 4:
			break;
		default:
			throw std::logic_error("Can't get an integer with that many bytes!");
	}
}


void Inspector::updatedData(QByteArray buf, qint64 timestamp) {
	QByteArray myPortion;
	myPortion.reserve(_length);
	if (!_bigEndian) {
		myPortion = buf.mid(_first, _length);
	} else {
		unsigned i;
		for (i = 0; i < _length; ++i) {
			myPortion.prepend(buf.at(_first + i));
		}

	}

	switch (_length) {
		case 1:
			_sendNewValue(timestamp, _signed ?
			              myPortion.at(0)
			              : getFromByteArray<vrpn_uint8>(myPortion));
			break;
		case 2:

			_sendNewValue(timestamp, _signed ?
			              getFromByteArray<vrpn_int16>(myPortion) :
			              getFromByteArray<vrpn_uint16>(myPortion));
			break;
		case 4:
			_sendNewValue(timestamp, _signed ?
			              getFromByteArray<vrpn_int32>(myPortion) :
			              getFromByteArray<vrpn_uint32>(myPortion));
			break;
	}
}

void Inspector::_sendNewValue(qint64 timestamp, float val) {
	emit newValue(val);
	emit newValue(float(timestamp) / 1000.0f, val);
}
