/*
 * 
 */

#ifndef CPC_H
#define CPC_H

#include <array>
#include <map>
#include <regex>
#include <stdexcept>
#include <tuple>
#include <numeric>
#include <mutex>
#include <iterator>
#include <set>

//dependencies
#include <jansson.h>
#include <curl/curl.h>
#include <uuid/uuid.h>

//debug
#include <iostream>
//#define NDEBUG
#include <cassert>

#define CPC_NODE_COUNT 25
#define CPC_DOMAIN_NAME_SIZE (256 * 1024)
#define CPC_TIMEOUT 1000
#define CPC_CONTIMEOUT 30L
#define CPC_TFRTIMEOUT 30L //transfer timeout. Включает connection
#define CPC_NODE_URL "https://raida%s.cloudcoin.global/service/%s"

namespace rot::cpc { //CloudPlusCoin. CloudCoin API		

// ----- report -----
	enum ResultCode {
		ok = 0b0,				//0
		error = 0b1,			//1
		undefined = 0b10,		//2
		parse_error = 0b100,	//4
		init_error = 0b1000,	//8
		request_error = 0b10000	//16
	};
	struct ResultRep {
		ResultCode code = undefined;
		// пустое или с разделителем \n, но без переноса в конце
		std::string expl = "";
	};
	class CPCError : public std::runtime_error {
		ResultCode code;
	public:
		explicit CPCError(ResultCode error_code, const std::string& expl) : std::runtime_error(expl) { code = error_code; }
		ResultCode get_code() const { return code; }
	};

// ----- coin structure -----
	using NetID = unsigned short;
	using NodeID = std::pair<NetID, size_t>; //network, node 
	using SNType = unsigned int;

	struct CCDate { //CloudCoin date
		unsigned short month = 0;
		unsigned short year = 0;

		std::string get_string() const { return std::to_string(month) + '-' + std::to_string(year); }
		bool set_string(std::string date);
	};
	struct CloudCoin {
		std::array<std::string, CPC_NODE_COUNT>	ans;	//authenticity numbers 25
		SNType						sn;		//serial number 1-16,777,216
		NetID						nn;		//network number 1-256
		CCDate						ed;		//expiration date CCDate
		char						pown[CPC_NODE_COUNT + 1];	//+1 for \0
		std::vector<std::string>	aoid;
	};

// ----- request -----
	using RawReqState = std::tuple<std::map<Response>, ResultRep>; //raw request state
	using ReqState = std::tuple<std::vector<NodeResponse>, ResultRep>;
	using Params = std::multimap<std::string, std::string>;

	//for connection pool safe and other cache
	CURLM* rhandle; //request handle
	void init_libcurl();

	struct Accumulator {
		char* data = NULL; // обязательно проинициализировать NULL'ом
		int size {0};
	};	
	void free_accumulators(Accumulator* accums, size_t rsize);

	size_t write_response(void* buffer, size_t size, size_t length, void* storage);
	struct Response {
		std::string content;
		ResultRep report;
	};
	RawReqState request(const std::vector<std::string>& urls); 

	std::string get_url(
			const NodeID& node, 
			const std::string& service, 
			const Params& params);

	class NodeResponse {
		ResultRep report; //about request successful
		//about node status if request was successful
		std::string server;
		std::string status;
		std::string message;
		std::string time;
	public:
		//NodeResponse() = delete;
		NodeResponse();
		NodeResponse(const Response& raw);
		ResultRep get_report() const { return report; }
		std::string get_server() const { return server; }
		std::string get_status() const { return status; }
		std::string get_message() const { return message; }
		std::string get_time() const { return time; }
	};

// ----- other -----
	using ReadState = std::tuple<std::vector<CloudCoin>, ResultRep>

	int coin_denomination(SNType sn);
	std::string guid();	
	std::string json_object_get_string(json_t*, const std::string&);	
	std::vector<NodeResponse> wrap_raw_responses(const std::vector<Response>&);

// ----- user area -----
	/*
	 * To establish that RAIDA's nodes is ready to "Detect" operation.
	 */
	ReqState echo(const std::vector<NodeID>& nodes, bool brief = false);

	/*
	 * 
	 */
	ReqState detect(CloudCoin& coin, const std::set<NodeID>* bl = nullptr);

	/*
	 *
	 */
	ReadState read_json(const std::string& path);

} //rot::cpc

#endif //CPC_H
