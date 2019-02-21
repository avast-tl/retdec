/**
 * @file retdec/include/bin2llvmir/providers/calling_convention/powerpc64.h
 * @brief Calling conventions of PowerPC64 architecture.
 * @copyright (c) 2017 Avast Software, licensed under the MIT license
 */

#ifndef RETDEC_BIN2LLVMIR_PROVIDERS_CALL_CONV_POWERPC64_H
#define RETDEC_BIN2LLVMIR_PROVIDERS_CALL_CONV_POWERPC64_H

#include "retdec/bin2llvmir/providers/calling_convention/calling_convention.h"

namespace retdec {
namespace bin2llvmir {

class PowerPC64CallingConvention: public CallingConvention
{
	// Ctors, dtors.
	//
	public:
		PowerPC64CallingConvention(const Abi* a);
		virtual ~PowerPC64CallingConvention();

	// Construcor method.
	//
	public:
		static CallingConvention::Ptr create(const Abi* a);

};

}
}

#endif
