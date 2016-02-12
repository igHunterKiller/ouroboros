// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// A fast, but limited atof implementation. It reports to be 5x faster than 
// stdlib atof. In benchmarking it seems to be ~3.2x on Intel i7 processors.

// Marcin Sokalski; assumed public source
// http://stackoverflow.com/questions/98586/where-can-i-find-the-worlds-fastest-atof-implementation

#pragma once
#include <cmath>

namespace ouro {

// $(CitedCodeBegin)
inline bool atof(const char** pp_str, float* val)
// (C)2009 Marcin Sokalski gumix@ghnet.pl - All rights reserved.
{
	const char* str = *pp_str;

	int hdr=0;
	while (str[hdr]==L' ')
		hdr++;

	int cur=hdr;

	bool negative=false;
	bool has_sign=false;

	if (str[cur]==L'+' || str[cur]==L'-')
	{
		if (str[cur]==L'-')
			negative=true;
		has_sign=true;
		cur++;
	}
	else
		has_sign=false;

	int quot_digs=0;
	int frac_digs=0;

	bool full=false;

	char period=0;
	int binexp=0;
	int decexp=0;
	unsigned long value=0;

	while (str[cur]>=L'0' && str[cur]<=L'9')
	{
		if (!full)
		{
			if (value>=0x19999999 && str[cur]-L'0'>5 || value>0x19999999)
			{
				full=true;
				decexp++;
			}
			else
				value=value*10+str[cur]-L'0';
		}
		else
			decexp++;

		quot_digs++;
		cur++;
	}

	if (str[cur]==L'.' || str[cur]==L',')
	{
		period=str[cur];
		cur++;

		while (str[cur]>=L'0' && str[cur]<=L'9')
		{
			if (!full)
			{
				if (value>=0x19999999 && str[cur]-L'0'>5 || value>0x19999999)
					full=true;
				else
				{
					decexp--;
					value=value*10+str[cur]-L'0';
				}
			}

			frac_digs++;
			cur++;
		}
	}

	if (!quot_digs && !frac_digs)
		return false;

	char exp_char=0;

	int decexp2=0; // explicit exponent
	bool exp_negative=false;
	bool has_expsign=false;
	int exp_digs=0;

	// even if value is 0, we still need to eat exponent chars
	if (str[cur]==L'e' || str[cur]==L'E')
	{
		exp_char=str[cur];
		cur++;

		if (str[cur]==L'+' || str[cur]==L'-')
		{
			has_expsign=true;
			if (str[cur]=='-')
				exp_negative=true;
			cur++;
		}

		while (str[cur]>=L'0' && str[cur]<=L'9')
		{
			if (decexp2>=0x19999999)
				return false;
			decexp2=10*decexp2+str[cur]-L'0';
			exp_digs++;
			cur++;
		}

		if (exp_negative)
			decexp-=decexp2;
		else
			decexp+=decexp2;
	}

	// end of str scan, cur contains value's tail

	if (value)
	{
		while (value<=0x19999999)
		{
			decexp--;
			value=value*10;
		}

		if (decexp)
		{
			// ensure 1bit space for mul by something lower than 2.0
			if (value&0x80000000)
			{
				value>>=1;
				binexp++;
			}

			if (decexp>308 || decexp<-307)
				return false;

			// convert exp from 10 to 2 (using FPU)
			int E;
			double v=pow(10.0,decexp);
			double m=frexp(v,&E);
			m=2.0*m;
			E--;
			value=(unsigned long)floor(value*m);

			binexp+=E;
		}

		binexp+=23; // rebase exponent to 23bits of mantisa


		// so the value is: +/- VALUE * pow(2,BINEXP);
		// (normalize manthisa to 24bits, update exponent)
		while (value&0xFE000000)
		{
			value>>=1;
			binexp++;
		}
		if (value&0x01000000)
		{
			if (value&1)
				value++;
			value>>=1;
			binexp++;
			if (value&0x01000000)
			{
				value>>=1;
				binexp++;
			}
		}

		while (!(value&0x00800000))
		{
			value<<=1;
			binexp--;
		}

		if (binexp<-127)
		{
			// underflow
			value=0;
			binexp=-127;
		}
		else
		if (binexp>128)
			return false;

		//exclude "implicit 1"
		value&=0x007FFFFF;

		// encode exponent
		unsigned long exponent=(binexp+127)<<23;
		value |= exponent;
	}

	// encode sign
	unsigned long sign=negative<<31;
	value |= sign;

	if (val)
	{
		*(unsigned long*)val=value;
	}

	*pp_str = &str[cur];
	return true;
}

// $(CitedCodeEnd)

inline bool atof(const char* str, float* val) { return atof(&str, val); }

}
