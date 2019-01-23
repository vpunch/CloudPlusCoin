/*
 * ЗАДАЧИ:
 * 3. Специализировать детект
 * 4. Исправить echo (ко всем)
 * 5. fix(принимает тикеты, делает запрос), ticket (смотит статус монет, выбирает схему, делает тект по схеме)
 * Проверить типы после всего
 */

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

#define CPC_NODE_COUNT 25 //в каждой сети по стандарту
#define CPC_DOMAIN_NAME_SIZE (256 * 1024)
#define CPC_TIMEOUT 1000
#define CPC_CONTIMEOUT 30L
#define CPC_TFRTIMEOUT 30L //transfer timeout. Включает connection

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
	using NodeNum = unsigned short;
	using NodeID = std::pair<NetID, size_t>; //network, node 
	using SNType = unsigned int; //serial number type
	using DnmType = unsigned short;

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
	using RawReqState = std::tuple<std::vector<Response>, ResultRep>; //raw request state
	RawReqState request(const std::vector<std::string>& urls); 

	using Params = std::multimap<std::string, std::string>;
	std::vector<std::string> get_url(
			const NodeID& node, 
			const std::string& service, 
			const Params& params);

	struct NodeDirectory {
		struct Address {
			std::string url = "";
			short port = 443;
			int delay = -1; //ms. Обновляется при echo
		};

		bool fails_echo = false;
		bool fails_detect = false;
		bool fails_fix = false;
		bool fails_ticket = false;
		std::string location = "";
		std::vector<Address> addresses;
	};
	using DirDump = std::map<NetID, std::array<NodeDirectory, CPC_NODE_COUNT>>;

	//expand content of raw request
	class NodeResponse {
		ResultRep report; //about request successful
		//about node status if request was successful
		std::string server;
		std::string status;
		std::string message;
		std::string time;
	public:
		NodeResponse() : report {undefined, ""} { }
		NodeResponse(const ResultRep& irep) : report(irep) { }
		NodeResponse(const Response& raw);
		ResultRep get_report() const { return report; }
		std::string get_server() const { return server; }
		std::string get_status() const { return status; }
		std::string get_message() const { return message; }
		std::string get_time() const { return time; }
	};

// ----- other -----	
	int coin_denomination(SNType sn);
	std::string guid();	
	std::string json_object_get_string(json_t*, const std::string&);	
	std::vector<NodeResponse> wrap_raw_responses(const std::vector<Response>&);
	std::vector<NetID> networks();

// ----- user area -----
// ----- low level -----
	using ReadState = std::tuple<std::vector<CloudCoin>, ResultRep>;
	using ReqState = std::tuple<std::vector<NodeResponse>, ResultRep>;

	std::mutex blmtx; //blacklist mutex (for url)

	ReqState ticket(const CloudCoin& coin, NodeID node, std::set<NodeID>* const bl = nullptr);

// ----- high level -----

	void directory();
	/*
	 * To establish that RAIDA's nodes is ready to "Detect" operation.
	 */
	ReqState echo(const std::vector<NodeID>& nodes, bool brief = false);

	/*
	 * 
	 */
	ReqState detect(CloudCoin& coin, std::set<NodeID>* const bl = nullptr);

	/*
	 *
	 */
	ReqState fix(CloudCoin& coin, std::set<NodeID>* const bl = nullptr);

	/*
	 *
	 */
	ReadState read_json(const std::string& path);

} //rot::cpc

#endif //CPC_H
