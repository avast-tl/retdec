/**
 * @file src/fileformat/file_format/pe/authenticode/pkcs7.h
 * @brief Class wrapper above openssl Pkcs7
 * @copyright (c) 2020 Avast Software, licensed under the MIT license
 */

#pragma once

#include "authenticode_structs.h"
#include "x509_certificate.h"
#include "pkcs9.h"

#include "retdec/fileformat/types/certificate_table/certificate_table.h"

#include <openssl/bn.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/ocsp.h>
#include <openssl/pkcs7.h>
#include <openssl/ts.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/pkcs7.h>

#include <array>
#include <vector>
#include <string>
#include <cstdint>
#include <ctime>
#include <optional>
#include <cstdint>
#include <exception>

namespace authenticode {

class ContentInfo
{
	SpcIndirectDataContent* spcContent;
	SpcContentInfo *contentInfo;
public:
	std::string contentType;
	std::vector<std::uint8_t> digest;
	Algorithms digestAlgorithm;

	ContentInfo() = default;
	ContentInfo(const unsigned char* data, size_t length);
};

class Pkcs7
{
private:
	PKCS7* pkcs7;
	SpcIndirectDataContent* spcContent;
	std::optional<X509Certificate> signer;

	STACK_OF(X509)* getCertificates() const;
	STACK_OF(X509)* getSigners();

	void parseSignerInfo(PKCS7_SIGNER_INFO* si_info);
	void parseCertificates(PKCS7_SIGNER_INFO* info);

public:
	std::uint64_t version;
	ContentInfo contentInfo;

	std::vector<X509Certificate> certificates;
	std::vector<Pkcs7> nestedSignatures;
	std::vector<Pkcs9> counterSignatures;
	/* TODO add ms counter signatures */

	std::vector<retdec::fileformat::DigitalSignature> getSignatures() const;

	Pkcs7(std::vector<unsigned char> input);
	~Pkcs7();
};

} // namespace authenticode