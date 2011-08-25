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

// Library/third-party includes
// - none

// Standard includes
#include <stdexcept>
#include <iostream>
#include <stdint.h>

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


void Inspector::updatedData(QByteArray buf) {
	QByteArray myPortion;
	myPortion.reserve(_length);
	if (!_bigEndian) {
		myPortion = buf.mid(_first, _length);
	} else {
		for (int i = 0; i < _length; ++i) {
			myPortion.prepend(buf.at(_first + i));
		}

	}

	switch (_length) {
		case 1:
			_sendNewValue(_signed ?
			              myPortion.at(0)
			              : *reinterpret_cast<uint8_t *>(myPortion.data()));
			break;
		case 2:

			_sendNewValue(_signed ?
			              *reinterpret_cast<int16_t *>(myPortion.data()) :
			              *reinterpret_cast<uint16_t *>(myPortion.data()));
			break;
		case 4:
			_sendNewValue(_signed ?
			              *reinterpret_cast<int32_t *>(myPortion.data()) :
			              *reinterpret_cast<uint32_t *>(myPortion.data()));
			break;
	}
}

void Inspector::_sendNewValue(float val) {
	emit newValue(val);
}
