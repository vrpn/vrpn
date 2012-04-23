/** @file
	@brief Header

	@date 2012

	@author
	Jan Ciger
	<jan.ciger@reviatech.com>
	
*/

//           Copyright Reviatech 2012.
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#ifndef INCLUDED_vrpn_OneEuroFilter_h_GUID_C56A0525_3809_44B7_AA16_98711638E762
#define INCLUDED_vrpn_OneEuroFilter_h_GUID_C56A0525_3809_44B7_AA16_98711638E762


// Internal Includes
// - none

// Library/third-party includes
// - none

// Standard includes
// - none


#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif


// "One Euro" filter for reducing jitter
// http://hal.inria.fr/hal-00670496/

class LowPassFilterVec
{
public:
	LowPassFilterVec() : _firstTime(true) 
	{
	}

	const vrpn_float64 *filter(const vrpn_float64 *x, vrpn_float64 alpha)
	{
		if(_firstTime)
		{
			_firstTime = false;
			memcpy(_hatxprev, x, sizeof(vrpn_float64) * 3);
		}

		vrpn_float64 hatx[3];
		hatx[0] = alpha * x[0] + (1 - alpha) * _hatxprev[0];
		hatx[1] = alpha * x[1] + (1 - alpha) * _hatxprev[1];
		hatx[2] = alpha * x[2] + (1 - alpha) * _hatxprev[2];

		memcpy(_hatxprev, hatx, sizeof(vrpn_float64) * 3);
		return _hatxprev;
	}

	const vrpn_float64 *hatxprev() { return _hatxprev; }

private:
	bool _firstTime;
	vrpn_float64 _hatxprev[3];
};

class OneEuroFilterVec
{
public:
	OneEuroFilterVec(vrpn_float64 rate, vrpn_float64 mincutoff, vrpn_float64 beta, LowPassFilterVec &xfilt, vrpn_float64 dcutoff, LowPassFilterVec &dxfilt) :
		_firstTime(true), _xfilt(xfilt), _dxfilt(dxfilt),
		_mincutoff(mincutoff), _beta(beta), _dcutoff(dcutoff), _rate(rate) {};

	const vrpn_float64 *filter(const vrpn_float64 *x)
	{
		vrpn_float64 dx[3];

		if(_firstTime)
		{
			_firstTime = false;
			dx[0] = dx[1] = dx[2] = 0.0f;

		} else {
			const vrpn_float64 *filtered_prev = _xfilt.hatxprev();
			dx[0] = (x[0] - filtered_prev[0]) * _rate;
			dx[1] = (x[1] - filtered_prev[1]) * _rate;
			dx[2] = (x[2] - filtered_prev[2]) * _rate;
		}

		const vrpn_float64 *edx = _dxfilt.filter(dx, alpha(_rate, _dcutoff));
		vrpn_float64 norm = sqrt(edx[0]*edx[0] + edx[1]*edx[1] + edx[2]*edx[2]);
		vrpn_float64 cutoff = _mincutoff + _beta * norm;

		return _xfilt.filter(x, alpha(_rate, cutoff));
	}

private:
	vrpn_float64 alpha(vrpn_float64 rate, vrpn_float64 cutoff)
	{
		vrpn_float64 tau = (vrpn_float64)(1.0f/(2.0f * M_PI * cutoff));
		vrpn_float64 te  = 1.0f/rate;
		return 1.0f/(1.0f + tau/te);
	}

	bool _firstTime;
	vrpn_float64 _rate;
	vrpn_float64 _mincutoff, _dcutoff;
	vrpn_float64 _beta;
	LowPassFilterVec &_xfilt, &_dxfilt;
};

class LowPassFilterQuat
{
public:
	LowPassFilterQuat() : _firstTime(true) 
	{
	}

	const double *filter(const q_type x, vrpn_float64 alpha)
	{
		if(_firstTime)
		{
			_firstTime = false;
			q_copy(_hatxprev, x);
		}

		q_type hatx;
		q_slerp(hatx, _hatxprev, x, alpha);
		q_copy(_hatxprev, hatx);
		return _hatxprev;
	}

	const double *hatxprev() { return _hatxprev; }

private:
	bool _firstTime;
	q_type _hatxprev;
};

class OneEuroFilterQuat
{
public:
	OneEuroFilterQuat(vrpn_float64 rate, vrpn_float64 mincutoff, vrpn_float64 beta, LowPassFilterQuat &xfilt, vrpn_float64 dcutoff, LowPassFilterQuat &dxfilt) :
		_firstTime(true), _xfilt(xfilt), _dxfilt(dxfilt),
		_mincutoff(mincutoff), _beta(beta), _dcutoff(dcutoff), _rate(rate) {};

	const double *filter(const q_type x)
	{
		q_type dx;

		if(_firstTime)
		{
			_firstTime = false;
			dx[0] = dx[1] = dx[2] = 0;
			dx[3] = 1;

		} else {
			q_type filtered_prev;
			q_copy(filtered_prev, _xfilt.hatxprev());

			q_type inverse_prev;
			q_invert(inverse_prev, filtered_prev);
			q_mult(dx, x, inverse_prev);
		}

		q_type edx;
		q_copy(edx, _dxfilt.filter(dx, alpha(_rate, _dcutoff)));
		
		// avoid taking acos of an invalid number due to numerical errors
		if(edx[Q_W] > 1.0) edx[Q_W] = 1.0;
		if(edx[Q_W] < -1.0) edx[Q_W] = -1.0;		

		double ax,ay,az,angle;
		q_to_axis_angle(&ax, &ay, &az, &angle, edx);
		
		vrpn_float64 cutoff = _mincutoff + _beta * angle;

		const double *result = _xfilt.filter(x, alpha(_rate, cutoff));

		return result;
	}

private:
	vrpn_float64 alpha(vrpn_float64 rate, vrpn_float64 cutoff)
	{
		vrpn_float64 tau = (vrpn_float64)(1.0f/(2.0f * M_PI * cutoff));
		vrpn_float64 te  = 1.0f/rate;
		return 1.0f/(1.0f + tau/te);
	}

	bool _firstTime;
	vrpn_float64 _rate;
	vrpn_float64 _mincutoff, _dcutoff;
	vrpn_float64 _beta;
	LowPassFilterQuat &_xfilt, &_dxfilt;
};


#endif // INCLUDED_vrpn_OneEuroFilter_h_GUID_C56A0525_3809_44B7_AA16_98711638E762

