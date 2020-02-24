/**
* @file include/retdec/bin2llvmir/optimizations/x87_fpu/x87_fpu.h
* @brief x87 FPU analysis - replace fpu stack operations with FPU registers.
* @copyright (c) 2019 Avast Software, licensed under the MIT license
*/

#ifndef RETDEC_BIN2LLVMIR_OPTIMIZATIONS_X87_FPU_X87_FPU_H
#define RETDEC_BIN2LLVMIR_OPTIMIZATIONS_X87_FPU_X87_FPU_H

#include <map>

#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

#include "eigen/include/Eigen/Dense"

#include "retdec/bin2llvmir/analyses/symbolic_tree.h"
#include "retdec/bin2llvmir/providers/abi/abi.h"
#include "retdec/bin2llvmir/providers/config.h"

#define EMPTY_FPU_STACK 8
#define FULL_FPU_STACK 0
#define RETURN_VALUE_PASSED_THROUGH_ST0 7
#define UNKNOWN_CALLING_CONVENTION -1
#define INCONSISTENT_CALLING_CONVENTION -2
#define INCORRECT_STACK_OPERATION -3
#define ANALYZE_FAIL false
#define ANALYZE_SUCCESS true

namespace retdec {
namespace bin2llvmir {

class FunctionAnalyzeMetadata
{
	public:

		bool analyzeSuccess = true;
		enum IndexType {
			inIndex, outIndex
		};

		llvm::Function& function;
		std::map<llvm::BasicBlock*, std::map<IndexType,unsigned >> indexes;

		std::list<llvm::BasicBlock*> terminatingBasicBlocks;
		// A * x = B
		Eigen::MatrixXd A;
		Eigen::MatrixXd B;
		Eigen::MatrixXd x;

		int numberOfEquations = 0;

		// 1. index to register, 2.pseudo instruction
		std::list<std::pair<uint32_t ,llvm::Instruction&>> pseudoCalls;

	void addEquation(std::list<std::tuple<llvm::BasicBlock&,int,IndexType >> vars, int result);
	FunctionAnalyzeMetadata(llvm::Function &function1) : function(function1) {};

};

class X87FpuAnalysis : public llvm::ModulePass
{
	public:
		static char ID;
		X87FpuAnalysis();
		virtual bool runOnModule(llvm::Module& m) override;
		bool runOnModuleCustom(
				llvm::Module& m,
				Config* c,
				Abi* a);

	private:
		bool run();
		bool analyzeBasicBlock(
				FunctionAnalyzeMetadata& funMd,
				llvm::BasicBlock* bb,
				int& outTop);
		bool analyzeInstruction(
				FunctionAnalyzeMetadata& funMd,
				llvm::Instruction& i,
				std::list<int>& topVals,
				int& outTop);
	std::list<FunctionAnalyzeMetadata> getFunctions2Analyze();

	void printBlocksAnalyzeResult();
	/**
	 * Replace all FPU pseudo load and store function by load and store with concrete FPU registers.
	 */
	bool optimizeAnalyzedFpuInstruction();
	int expectedTopBasedOnCallingConvention(llvm::Instruction& inst);
	int expectedTopBasedOnRestOfBlock(llvm::Instruction& analyzedInstr);
	bool validTopForTerminatingBasicBlock(int top);
	int matrixRank(Eigen::MatrixXd &mat);
	bool consistenSystem(Eigen::MatrixXd &A, Eigen::MatrixXd &B);
	bool isFpuStackTopValidForActualArchitectureAndCallingConvention(llvm::Function* function);
	int getFpuStackTopForTerminatingBlockOfX86_32Arch(int top);

	bool isInstructionFunctionCall(llvm::Value* inst);
	/**
	 * @pre Expect first call isInstructionFunctionCall().
	 */
	bool isFunctionReturnTypeFloatingPoint(llvm::Value* inst);

	private:

		llvm::Module* _module = nullptr;
		Config* _config = nullptr;
		Abi* _abi = nullptr;
		llvm::GlobalVariable* top = nullptr;

		std::list<FunctionAnalyzeMetadata> analyzedFunctionsMetadata; //functions where detected FPU stack access

};

} // namespace bin2llvmir
} // namespace retdec

#endif
