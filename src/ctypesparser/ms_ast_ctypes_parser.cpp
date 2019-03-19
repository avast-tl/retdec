#include <cassert>
#include <retdec/utils/container.h>
#include <sstream>

#include "llvm/Demangle/MicrosoftDemangleNodes.h"
#include "llvm/Demangle/MicrosoftDemangle.h"

#include "retdec/ctypesparser/ms_ast_ctypes_parser.h"
#include "retdec/ctypes/module.h"
#include "retdec/ctypes/unknown_type.h"
#include "retdec/ctypes/type.h"
#include "retdec/ctypes/function.h"
#include "retdec/ctypes/parameter.h"
#include "retdec/ctypes/unknown_type.h"
#include "retdec/ctypes/integral_type.h"
#include "retdec/ctypes/floating_point_type.h"
#include "retdec/ctypes/pointer_type.h"
#include "retdec/ctypes/reference_type.h"
#include "retdec/ctypes/void_type.h"

namespace retdec {
namespace ctypesparser {

namespace {

std::string printToString(llvm::ms_demangle::Node *node)
{

	llvm::itanium_demangle::OutputStream s;
	char *buf = nullptr;

	if (!node) {
		return {};
	}

	if (!initializeOutputStream(buf, nullptr, s, 1024)) {
		return {};
	}

	node->output(s, llvm::ms_demangle::OutputFlags::OF_Default);
	s += '\0';
	buf = s.getBuffer();

	std::string name(buf);
	free(buf);
	return name;
}

}    // anonymous namespace

using Kind = llvm::ms_demangle::NodeKind;

bool MsToCtypesParser::parseInto(
	llvm::ms_demangle::SymbolNode *ast,
	std::unique_ptr<retdec::ctypes::Module> &module,
	const retdec::ctypesparser::CTypesParser::TypeWidths &typeWidths,
	const retdec::ctypesparser::CTypesParser::TypeSignedness &typeSignedness,
	const retdec::ctypes::CallConvention &callConvention)
{
	assert(ast && "Ast cannot be null");

	context = module->getContext();
	defaultCallConv = callConvention;
	this->typeWidths = typeWidths;
	this->typeSignedness = typeSignedness;

	switch (ast->kind()) {
	case Kind::FunctionSymbol: {
		auto func = parseFunction(static_cast<llvm::ms_demangle::FunctionSymbolNode *>(ast));
		if (func) {
			module->addFunction(func);
			return true;
		}
		break;
	}
	default:
		break;
	}

	return false;
}

std::shared_ptr<ctypes::Function> MsToCtypesParser::parseFunction(
	llvm::ms_demangle::FunctionSymbolNode *functionSymbolNode)
{
	auto funcSignature = functionSymbolNode->Signature;
	if (!funcSignature) {
		return nullptr;
	}

	std::string name = parseFunctionName(functionSymbolNode->Name);
	std::shared_ptr<ctypes::Type> returnType = parseType(funcSignature->ReturnType);
	ctypes::CallConvention callConvention = parseCallConvention(funcSignature->CallConvention);
	ctypes::Function::Parameters parameters = parseFunctionParameters(funcSignature->Params);
	ctypes::Function::VarArgness varArgness = parseVarArgness(funcSignature->IsVariadic);

	return ctypes::Function::create(context, name, returnType, parameters, callConvention, varArgness);
}

std::string MsToCtypesParser::parseFunctionName(llvm::ms_demangle::QualifiedNameNode *nameNode)
{
	assert(nameNode && "Violated precondition.");

	return printToString(nameNode);
}

ctypes::CallConvention MsToCtypesParser::parseCallConvention(
	llvm::ms_demangle::CallingConv callConv)
{
	using Conv = llvm::ms_demangle::CallingConv;

	switch (callConv) {
	case Conv::Cdecl:
		return {"cdecl"};
	case Conv::Pascal:
		return {"pascal"};
	case Conv::Thiscall:
		return {"thiscall"};
	case Conv::Stdcall:
		return {"stdcall"};
	case Conv::Fastcall:
		return {"fastcall"};
	case Conv::Clrcall:
		return {"clrcall"};
	case Conv::Eabi:
		return {"eabi"};
	case Conv::Vectorcall:
		return {"vectorcall"};
	case Conv::Regcall:
		return {"regcall"};
	default:
		return {};
	}
}

std::shared_ptr<ctypes::Type> MsToCtypesParser::parseType(llvm::ms_demangle::Node *typeNode)
{
	using NodeKind = llvm::ms_demangle::NodeKind;

	if (!typeNode) {
		return std::static_pointer_cast<ctypes::Type>(ctypes::UnknownType::create());
	}

	switch (typeNode->kind()) {
	case NodeKind::PrimitiveType:
		return parsePrimitiveType(static_cast<llvm::ms_demangle::PrimitiveTypeNode *> (typeNode));
	case NodeKind::PointerType:
		return parsePointerType(static_cast<llvm::ms_demangle::PointerTypeNode *> (typeNode));
	case NodeKind::TagType:
		return parseNamedType(static_cast<llvm::ms_demangle::TagTypeNode *>(typeNode)->QualifiedName);
	case NodeKind::FunctionSignature:
	case NodeKind::ThunkSignature:
		return parseFuncType(static_cast<llvm::ms_demangle::FunctionSignatureNode *>(typeNode));
	default:
		break;
	}

	return std::static_pointer_cast<ctypes::Type>(ctypes::UnknownType::create());
}

std::shared_ptr<ctypes::Type> MsToCtypesParser::parsePrimitiveType(
	llvm::ms_demangle::PrimitiveTypeNode *primitiveTypeNode)
{
	assert(primitiveTypeNode && "Violated precondition.");

	using PrimitiveKind = llvm::ms_demangle::PrimitiveKind;

	switch (primitiveTypeNode->PrimKind) {
	case PrimitiveKind::Void:
		return ctypes::VoidType::create();
	case PrimitiveKind::Bool:
	case PrimitiveKind::Char:
	case PrimitiveKind::Schar:
	case PrimitiveKind::Uchar:
	case PrimitiveKind::Char16:
	case PrimitiveKind::Char32:
	case PrimitiveKind::Short:
	case PrimitiveKind::Ushort:
	case PrimitiveKind::Int:
	case PrimitiveKind::Uint:
	case PrimitiveKind::Long:
	case PrimitiveKind::Ulong:
	case PrimitiveKind::Int64:
	case PrimitiveKind::Uint64:
	case PrimitiveKind::Wchar:
		return parseIntegralType(primitiveTypeNode);
	case PrimitiveKind::Float:
	case PrimitiveKind::Double:
	case PrimitiveKind::Ldouble:
		return parseFloatingPointType(primitiveTypeNode);
	case PrimitiveKind::Nullptr:
	default:
		return ctypes::UnknownType::create();
	}
}

std::shared_ptr<ctypes::IntegralType> MsToCtypesParser::parseIntegralType(
	llvm::ms_demangle::PrimitiveTypeNode *integralTypeNode)
{
	assert(integralTypeNode && "Violated precondition.");

	std::string name = getTypeName(integralTypeNode->PrimKind);
	unsigned bitWidth = retdec::utils::mapGetValueOrDefault(typeWidths, name, defaultBitWidth);
	ctypes::IntegralType::Signess signess = toSigness(name);

	return ctypes::IntegralType::create(context, name, bitWidth, signess);
}

std::shared_ptr<ctypes::FloatingPointType> MsToCtypesParser::parseFloatingPointType(
	llvm::ms_demangle::PrimitiveTypeNode *floatingPointTypeNode)
{
	assert(floatingPointTypeNode && "Violated precondition.");

	std::string name = getTypeName(floatingPointTypeNode->PrimKind);
	unsigned bitWidth = retdec::utils::mapGetValueOrDefault(typeWidths, name, defaultBitWidth);

	return ctypes::FloatingPointType::create(context, name, bitWidth);
}

ctypes::Function::Parameters MsToCtypesParser::parseFunctionParameters(
	llvm::ms_demangle::NodeArrayNode *parameters)
{
	ctypes::Function::Parameters params;

	if (parameters) {
		for (size_t i = 0; i < parameters->Count; ++i) {
			if (parameters->Nodes[i]) {                // for safety
				auto type = parseType(dynamic_cast<llvm::ms_demangle::TypeNode *>(parameters
					->Nodes[i]));
				auto param = ctypes::Parameter("param" + std::to_string(i), type);
				params.emplace_back(param);
			}
		}
	}

	return params;
}

ctypes::FunctionType::VarArgness MsToCtypesParser::parseVarArgness(bool isVarArg)
{    // TODO to parent class
	if (isVarArg) {
		return ctypes::FunctionType::VarArgness::IsVarArg;
	} else {
		return ctypes::FunctionType::VarArgness::IsNotVarArg;
	}
}

std::shared_ptr<ctypes::Type> MsToCtypesParser::parsePointerType(
	llvm::ms_demangle::PointerTypeNode *typeNode)
{
	using Affinity = llvm::ms_demangle::PointerAffinity;

	if (!typeNode->Pointee) {
		return ctypes::UnknownType::create();
	}

	auto pointee = parseType(typeNode->Pointee);

	switch (typeNode->Affinity) {
	case Affinity::Pointer: {
		unsigned bitWidth = utils::mapGetValueOrDefault(typeWidths, "pointer", defaultBitWidth);
		return ctypes::PointerType::create(context, pointee, bitWidth);
	}
	case Affinity::Reference: {
		unsigned bitWidth = utils::mapGetValueOrDefault(typeWidths, "pointer", defaultBitWidth);
		return ctypes::ReferenceType::create(context, pointee, bitWidth);
	}
	case Affinity::RValueReference:
		// TODO
	default:
		return ctypes::UnknownType::create();
	}
}

std::shared_ptr<ctypes::Type> MsToCtypesParser::parseNamedType(llvm::ms_demangle::Node *node)
{
	std::string name = printToString(node);

	if (name.empty()) {
		return std::static_pointer_cast<ctypes::Type>(ctypes::UnknownType::create());
	} else {
		return std::static_pointer_cast<ctypes::Type>(ctypes::NamedType::create(context, name));
	}
}

std::string MsToCtypesParser::getTypeName(llvm::ms_demangle::PrimitiveKind type) const
{
	using PrimitiveKind = llvm::ms_demangle::PrimitiveKind;

	std::map<llvm::ms_demangle::PrimitiveKind, std::string> typeMap{
		{PrimitiveKind::Bool, "bool"},
		{PrimitiveKind::Char, "char"},
		{PrimitiveKind::Schar, "signed char"},
		{PrimitiveKind::Uchar, "unsigned char"},
		{PrimitiveKind::Char16, "char16_t"},
		{PrimitiveKind::Char32, "char32_t"},
		{PrimitiveKind::Short, "short"},
		{PrimitiveKind::Ushort, "unsigned short"},
		{PrimitiveKind::Int, "int"},
		{PrimitiveKind::Uint, "unsigned int"},
		{PrimitiveKind::Long, "long"},
		{PrimitiveKind::Ulong, "unsigned long"},
		{PrimitiveKind::Int64, "int64_t"},
		{PrimitiveKind::Uint64, "uint64_t"},
		{PrimitiveKind::Wchar, "wchar_t"},
		{PrimitiveKind::Float, "float"},
		{PrimitiveKind::Double, "double"},
		{PrimitiveKind::Ldouble, "long double"}
	};

	return typeMap[type];
}

std::shared_ptr<ctypes::FunctionType> MsToCtypesParser::parseFuncType(
	llvm::ms_demangle::FunctionSignatureNode *funcSignature)
{
	auto callingConvention = parseCallConvention(funcSignature->CallConvention);
	auto returnType = parseType(funcSignature->ReturnType);
	auto parameters = parseFuncTypeParameters(funcSignature->Params);
	auto varArgness = parseVarArgness(funcSignature->IsVariadic);

	return ctypes::FunctionType::create(context, returnType, parameters, callingConvention, varArgness);
}

ctypes::FunctionType::Parameters MsToCtypesParser::parseFuncTypeParameters(
	llvm::ms_demangle::NodeArrayNode *parameters)
{
	ctypes::FunctionType::Parameters params;

	if (parameters) {
		for (size_t i = 0; i < parameters->Count; ++i) {
			if (parameters->Nodes[i]) {                // for safety
				auto type = parseType(dynamic_cast<llvm::ms_demangle::TypeNode *>(parameters->Nodes[i]));
				params.emplace_back(type);
			}
		}
	}

	return params;
}

}    // namespace ctypesparser
}    // namespace retdec