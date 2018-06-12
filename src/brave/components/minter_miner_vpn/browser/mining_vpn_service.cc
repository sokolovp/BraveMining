/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/minter_miner_vpn/browser/mining_vpn_service.h"

#include "brave/vendor/ad-block/socks.h"
#include "brave/vendor/ad-block/socket.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <iostream>

#include "content/public/browser/browser_thread.h" 
#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/minter_miner_vpn/browser/dat_file_util.h"
//#include "brave/vendor/ad-block/ad_block_client.h"

#define DAT_FILE "ABPFilterParserData.dat"


#include "brave/vendor/ad-block/json.hpp"
#include "brave/vendor/ad-block/cryptonight.h"

void print_buffer(void *buffer, int length);
int init_input_buffer(uint8_t *input, char *input_file);
extern cryptonight_ctx* cryptonight_alloc_ctx(size_t use_fast_mem, size_t use_mlock, alloc_msg* msg);
extern void do_cryptonight_hash(const void* input, size_t len, void* output, cryptonight_ctx* ctx0);

#define HASH_OUTPUT_LENGTH 256
#define MAX_INPUT_LENGTH 1024

#define HASH_SIZE 32

inline unsigned char hf_hex2bin(char c, bool &err)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	else if (c >= 'a' && c <= 'f')
		return c - 'a' + 0xA;
	else if (c >= 'A' && c <= 'F')
		return c - 'A' + 0xA;

	err = true;
	return 0;
}

bool hex2bin(const char* in, unsigned int len, unsigned char* out)
{
	bool error = false;
	for (unsigned int i = 0; i < len; i += 2)
	{
		out[i / 2] = (hf_hex2bin(in[i], error) << 4) | hf_hex2bin(in[i + 1], error);
		if (error) return false;
	}
	return true;
}

inline char hf_bin2hex(unsigned char c)
{
	if (c <= 0x9)
		return '0' + c;
	else
		return 'a' - 0xA + c;
}

void bin2hex(const unsigned char* in, unsigned int len, char* out)
{
	for (unsigned int i = 0; i < len; i++)
	{
		out[i * 2] = hf_bin2hex((in[i] & 0xF0) >> 4);
		out[i * 2 + 1] = hf_bin2hex(in[i] & 0x0F);
	}
}

#define BUF_SZ 1024









using content::BrowserThread; 

namespace mining_vpn {

std::string MiningVpnService::g_vpn_component_id_(
    kVPNComponentId);
std::string MiningVpnService::g_vpn_component_base64_public_key_(
    kVPNComponentBase64PublicKey);

MiningVpnService::MiningVpnService()
    : BaseMinerVpnService(kVPNComponentName,
                              g_vpn_component_id_,
                              g_vpn_component_base64_public_key_)//,
//      ad_block_client_(new AdBlockClient()
{
          LOG(INFO) << "MiningVpnService::MiningVpnService";
}

MiningVpnService::~MiningVpnService() {
  Cleanup();
}

void MiningVpnService::Cleanup() {
  //ad_block_client_.reset();
}

bool MiningVpnService::ShouldStartRequest(const GURL& url,
    content::ResourceType resource_type,
    const std::string& tab_host) {
/*
  FilterOption current_option = FONoFilterOption;
  content::ResourceType internalResource = (content::ResourceType)resource_type;
  if (content::RESOURCE_TYPE_STYLESHEET == internalResource) {
    current_option = FOStylesheet;
  } else if (content::RESOURCE_TYPE_IMAGE == internalResource) {
    current_option = FOImage;
  } else if (content::RESOURCE_TYPE_SCRIPT == internalResource) {
    current_option = FOScript;
  }
*/
//  if (ad_block_client_->matches(url.spec().c_str(),
//        current_option,
//        tab_host.c_str())) {
    // LOG(ERROR) << "MiningVpnService::Check(), host: " << tab_host
    //  << ", resource type: " << resource_type
    //  << ", url.spec(): " << url.spec();
//    return false;
//  }

  return true;
}

#define BUF_SZ 1024

int login(brave_shields::plain_socket* sock, std::string wallet, char* responce)
{
	char buf[BUF_SZ];
	snprintf(buf, sizeof(buf), "{\"method\":\"login\",\"params\":{\"login\":\"%s\",\"pass\":\"%s\",\"rigid\":\"%s\",\"agent\":\"%s\"},\"id\":1}\n",
		wallet.c_str(), "x", "test", "2.0");

	if (!sock->send(buf))
		return 0;

	//LOG(INFO) << "login: " << buf;
	return sock->recv(responce, BUF_SZ);
}

int share(brave_shields::plain_socket* sock, std::string minerId, std::string jobId, std::string nonce, std::string hash, char* responce)
{
	char buf[BUF_SZ];
	snprintf(buf, sizeof(buf), "{\"method\":\"submit\",\"params\":{\"id\":\"%s\",\"job_id\":\"%s\",\"nonce\":\"%s\",\"result\":\"%s\"},\"id\":1}\n",
		minerId.c_str(), jobId.c_str(), nonce.c_str(), hash.c_str());

	if (!sock->send(buf))
		return 0;
	//LOG(INFO) << "send: " << hash;
	return sock->recv(responce, BUF_SZ);
}

void printNon(int non)
{
 const double th = 1000;
 double n = double(non);
 int a = n / th;
 if ((n - a * th) == 0)
  std::cout << "nonce = " << non << std::endl;
}

void mythread() { 

	brave_shields::sock_init();
	brave_shields::plain_socket S;
	S.set_hostname("monero.miners.pro:3333");
	//S.set_hostname("94.130.229.152:3333");
LOG(INFO) << "1:";
	if (!S.connect()) {
		LOG(INFO) << "connect";
		return ;
	}

	char buf[BUF_SZ];
	unsigned char binBlob[BUF_SZ];

	std::string w("45AcsLsDXqT8QxDUh7Mom6gujNxaH3n1j6PrTRYhfB2wi4ftvrDHzQYJTFuz2DSDUjAsRqJuVz5BN1qrPL255oqWNV667St");
	int sz = login(&S, w, buf);
LOG(INFO) << "2:";
	std::string sBlob;
	std::string sTarget;
	std::string sJobId;
	std::string sMinerId;

	uint32_t tarVal = 0;
	int len = 0;

	cryptonight_ctx* ctx0;
	ctx0 = cryptonight_alloc_ctx(0, 0, NULL); // values set for testing..
LOG(INFO) << "3:";
	uint8_t *output;	  //hash value 
	output = (uint8_t *)malloc(HASH_OUTPUT_LENGTH);

	uint32_t* pHashVal = (uint32_t*)(output + 28);
	uint32_t* pNonce = (uint32_t*)(binBlob + 39);

LOG(INFO) << "4:";
	while (true)
	{
		std::string datBuf(buf, sz);
		std::istringstream stream(datBuf);
		std::string dat;
		nlohmann::json jObj;

		while (std::getline(stream, dat)) //New jobs may come as chunks of buffer. Need to parse.
		{
			jObj = nlohmann::json::parse(dat.c_str(), nullptr, false);
			if (jObj.is_object())
				break;
		}
//LOG(INFO) << "5:";
		if (!jObj.is_object())
		{
			sz = login(&S, w, buf);
            Sleep(5000);
			continue;
		}
//LOG(INFO) << "6:";
		std::cout << "print1: " << jObj.dump() << std::endl;

		auto jError = jObj.find("error");
		if (jError != jObj.end() && jError->is_structured())
		{
			Sleep(5000);
			sz = login(&S, w, buf);
			continue;
		}
		LOG(INFO) << "7:";
		
		auto jResult = jObj.find("result");
		auto jParams = jObj.find("params");
		//First job after login or share confirmation
		if (jResult != jObj.end())
		{
			auto jJob = jResult->find("job");
			if (jJob != jResult->end())
			{
LOG(INFO) << "11:";
				sBlob		= jJob->value("blob", "0");
				sTarget		= jJob->value("target", "0");
				sJobId		= jJob->value("job_id", "0");
				sMinerId	= jJob->value("id", "0");
LOG(INFO) << "12:";

				hex2bin(sTarget.c_str(), sTarget.length(), (unsigned char*)&tarVal);
				hex2bin(sBlob.c_str(), sBlob.length(), binBlob);
				len = sBlob.length() / 2; //const?
			}
			//else
			// Share accepted
		}
		else if (jParams != jObj.end()) // New job
		{
LOG(INFO) << "13:";
			sBlob		= jParams->value("blob", "0");
			sTarget		= jParams->value("target", "0");
			sJobId		= jParams->value("job_id", "0");
			sMinerId	= jParams->value("id", "0");

			hex2bin(sTarget.c_str(), sTarget.length(), (unsigned char*)&tarVal);
			hex2bin(sBlob.c_str(), sBlob.length(), binBlob);
			len = sBlob.length() / 2;
LOG(INFO) << "14:";
		}
		else
		{// Smth unexpected
LOG(INFO) << "15:";
			sz = login(&S, w, buf);
LOG(INFO) << "16:";
			
			continue;
		}
LOG(INFO) << "8: " << sTarget;
printNon(tarVal);
		while (true)
		{
			do_cryptonight_hash((const uint8_t *)binBlob, len, output, ctx0);

			if (*pHashVal < tarVal)
				break;
			(*pNonce)++;
			printNon((*pNonce));
		}
LOG(INFO) << "9:";
		char non[9];
		char Result[65];

		bin2hex(output, 32, Result);
		std::string hash(Result, 64);

		bin2hex((unsigned char*)pNonce, 4, non);
		std::string nonce(non, 8);

		sz = share(&S, sMinerId, sJobId, nonce, hash, buf);
	}
LOG(INFO) << "10:";
	free(output);
	S.close(true);

}

bool MiningVpnService::Init() {

LOG(INFO) << "MiningVpnService::Init";
//oRecvThd = new std::thread(&mythread);

/*
    LOG(INFO) << "MiningVpnService::Init";
    LOG(INFO) << "install dir: " << install_dir;
    base::FilePath dat_file_path = install_dir.AppendASCII(DAT_FILE);
    if (!GetDATFileData(dat_file_path, buffer_)) {
        LOG(ERROR) << "File Mining doesn't exist";
        std::string str = "{user:name}";
        SetDATFileData(dat_file_path, str);
        GetDATFileData(dat_file_path, buffer_);
        LOG(INFO) << "buffer: " << buffer_;
    }
    if (buffer_.empty()) {
        LOG(ERROR) << "Could not obtain ad block data";
        return true;
    }*/
  return true;
}

void MiningVpnService::OnComponentReady(const std::string& component_id,
                                      const base::FilePath& install_dir) {
                                          
  LOG(INFO) << "MiningVpnService::OnComponentReady";
  
  /*base::FilePath dat_file_path = install_dir.AppendASCII(DAT_FILE);
  if (!GetDATFileData(dat_file_path, buffer_)) {
    LOG(ERROR) << "Could not obtain ad block data file";
    return;
  }
  if (buffer_.empty()) {
    LOG(ERROR) << "Could not obtain ad block data";
    return;
  }*/
/*  ad_block_client_.reset(new AdBlockClient());
  if (!ad_block_client_->deserialize((char*)&buffer_.front())) {
    ad_block_client_.reset();
    LOG(ERROR) << "Failed to deserialize ad block data";
    return;
  }
  */
  //	oRecvThd = new std::thread(&mythread);
	//oRecvThd->join();
    LOG(INFO) << "MiningVpnService::OnComponentReady";
}

// static
void MiningVpnService::SetComponentIdAndBase64PublicKeyForTest(
    const std::string& component_id,
    const std::string& component_base64_public_key) {
  g_vpn_component_id_ = component_id;
  g_vpn_component_base64_public_key_ = component_base64_public_key;
}

///////////////////////////////////////////////////////////////////////////////

// The brave shields factory. Using the Brave Shields as a singleton
// is the job of the browser process.
std::unique_ptr<MiningVpnService> MiningVpnServiceFactory() {
  return base::MakeUnique<MiningVpnService>();
}

}  
























