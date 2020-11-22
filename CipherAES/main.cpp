#include <cstring>
#include <iostream>
#include <fstream>
#include <string>

#include <openssl/evp.h>

constexpr size_t BLOCK_BYTES = 16;
constexpr size_t KEY_BYTES = 16;

using ubyte = unsigned char;

std::string encrypt(const std::string& plaintext,
	const ubyte key[KEY_BYTES], const ubyte iv[BLOCK_BYTES])
{
	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (EVP_EncryptInit_ex(ctx, EVP_aes_128_ctr(), nullptr, key, iv) != 1)
		throw std::runtime_error("Error at EVP_EncryptInit_ex");

	const ubyte* plainBuff = reinterpret_cast<const ubyte*>(plaintext.data());
	ubyte* cipherBuff = new ubyte[plaintext.size() + BLOCK_BYTES];
	const int plainSize = plaintext.size();
	int cipherSize = 0, tailSize;

	if (EVP_EncryptUpdate(ctx, cipherBuff, &cipherSize,
		plainBuff, plainSize) != 1)
		throw std::runtime_error("Error at EVP_EncryptUpdate");
	if (EVP_EncryptFinal_ex(ctx, cipherBuff + cipherSize, &tailSize) != 1)
		throw std::runtime_error("Error at EVP_EncryptFinal_ex");
	cipherSize += tailSize;

	EVP_CIPHER_CTX_free(ctx);
	const std::string ciphertext(reinterpret_cast<const char*>(cipherBuff),
		cipherSize);
	delete[] cipherBuff;
	return ciphertext;
}

std::string decrypt(const std::string& ciphertext,
	const ubyte key[KEY_BYTES], const ubyte iv[BLOCK_BYTES])
{
	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (EVP_DecryptInit_ex(ctx, EVP_aes_128_ctr(), nullptr, key, iv) != 1)
		throw std::runtime_error("Error at EVP_DecryptInit_ex");

	const ubyte* cipherBuff = reinterpret_cast<const ubyte*>(ciphertext.data());
	ubyte* plainBuff = new ubyte[ciphertext.size() + 1];
	const int cipherSize = ciphertext.size();
	int plainSize = 0, tailSize;

	if (EVP_DecryptUpdate(ctx, plainBuff, &plainSize,
		cipherBuff, cipherSize) != 1)
		throw std::runtime_error("Error at EVP_DecryptUpdate");
	if (EVP_DecryptFinal_ex(ctx, plainBuff + plainSize, &tailSize) != 1)
		throw std::runtime_error("Error at EVP_DecryptFinal_ex");
	plainSize += tailSize;

	EVP_CIPHER_CTX_free(ctx);
	const std::string plaintext(reinterpret_cast<const char*>(plainBuff),
		plainSize);
	delete[] plainBuff;
	return plaintext;
}

std::string encodeBase64(const std::string& str)
{
	const ubyte* strBuff = reinterpret_cast<const ubyte*>(str.data());
	ubyte* encBuff = new ubyte[((str.size() + 2) / 3) * 4 + 1];
	const int encSize = EVP_EncodeBlock(encBuff, strBuff, str.size());

	const std::string encoded(reinterpret_cast<const char*>(encBuff), encSize);
	delete[] encBuff;
	return encoded;
}

std::string decodeBase64(std::string enc)
{
	while (!enc.empty() && isspace(enc.back()))
		enc.pop_back();
	if (enc.empty())
		return "";

	const ubyte* encBuff = reinterpret_cast<const ubyte*>(enc.data());
	ubyte* decBuff = new ubyte[enc.size() + 1];
	int decSize = EVP_DecodeBlock(decBuff, encBuff, enc.size());
	if (enc.back() == '=')
	{
		--decSize;
		if (enc[enc.size() - 2] == '=')
			--decSize;
	}

	const std::string decoded(reinterpret_cast<const char*>(decBuff), decSize);
	delete[] decBuff;
	return decoded;
}

int main(int argc, char* argv[])
{
	if (argc >= 2 && std::strcmp(argv[1], "help") == 0)
	{
		std::cout << "Usage: " << std::endl;
		std::cout << "<program> help <params...>" << std::endl;
		std::cout << "  displays this help, further params are ignored" << std::endl;
		std::cout << "<program> e|d <filepath> <key> <iv>" << std::endl;
		std::cout << "  encrypts or decrypts (based on the first argument) file at "
			"<filepath> using AES-128-CTR algorithm with base64-encoding and given "
			"key <key> and initialization vector <iv>" << std::endl;
		return 0;
	}
	else if (argc != 5)
	{
		std::cerr << "You should give 5 input arguments. See help." << std::endl;
		return -1;
	}

	const std::string mode(argv[1]), filepath(argv[2]), key(argv[3]), iv(argv[4]);
	if (mode != "d" && mode != "e")
	{
		std::cerr << "Incorrect mode provided." << std::endl;
		return -1;
	}
	if (key.size() != KEY_BYTES || iv.size() != BLOCK_BYTES)
	{
		std::cerr << "Either key or initialization vector size is incorrect"
			<< std::endl;
		return -1;
	}
	std::ifstream file(filepath);
	if (!file.is_open())
	{
		std::cerr << "Error reading input file at: " << std::endl;
		std::cerr << "  " << filepath << std::endl;
		return -1;
	}
	std::string fileContents{ std::istreambuf_iterator<char>(file),
		std::istreambuf_iterator<char>() };

	const auto keyU = reinterpret_cast<const unsigned char*>(key.data()),
		       ivU  = reinterpret_cast<const unsigned char*>(iv.data());
	try
	{
		if (mode[0] == 'e')
			std::cout << encodeBase64(encrypt(fileContents, keyU, ivU)) << std::endl;
		else
			std::cout << decrypt(decodeBase64(fileContents), keyU, ivU) << std::endl;
	}
	catch (const std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	return 0;
}