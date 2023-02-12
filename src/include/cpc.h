/*
 * Специализировать детект
 * Исправить echo (ко всем)
 * fix(принимает тикеты, делает запрос), ticket (смотит статус монет, выбирает схему, делает тект по схеме)
 * Проверить типы после всего
 */



/*
 * Thread safe library
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

#define CPC_NODE_COUNT 25 //according to the standard in every network
#define CPC_DOMAIN_NAME_SIZE (256 * 1024)
#define CPC_TIMEOUT 1000
#define CPC_CONTIMEOUT 30L
#define CPC_TFRTIMEOUT 30L //transfer timeout (includes connection)

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
	struct CCDate { //CloudCoin date
		unsigned short month = 0;
		unsigned short year = 0;

		std::string get_string() const { return std::to_string(month) + '-' + std::to_string(year); }
		bool set_string(std::string date);
	};

	using SNType = unsigned int; //serial number type
	using NetID = unsigned short; //network id
	using DnmType = unsigned short; //denomination type

	struct CloudCoin {
		std::array<std::string, CPC_NODE_COUNT>	ans;	//authenticity numbers 25
		SNType						sn;		//serial number 1-16,777,216
		NetID						nn;		//network number 1-256
		CCDate						ed;		//expiration date
		char						pown[CPC_NODE_COUNT + 1];	//+1 for \0
		std::vector<std::string>	aoid;
	};

	int coin_denomination(SNType sn);
	std::string guid();


// -----
	//node не относится к service
	class NodeResponse {
		ResultRep report; //about request successful
		//about node status if request was successful
		std::string server;
		std::string status;
		std::string message;
		std::string time;
	public:
		NodeResponse() : report {undefined, ""} { }
		NodeResponse(const ResultRep& res_rep) : report(res_rep) { }
		ResultRep get_report() const { return report; }
		std::string get_server() const { return server; }
		std::string get_status() const { return status; }
		std::string get_message() const { return message; }
		std::string get_time() const { return time; }
	};
	
	using DirDump = std::map<NetID, std::array<NodeDirectory, CPC_NODE_COUNT>>;
	
	struct NodeDirectory {
		struct Address {
			std::string url = "";
			unsigned short port = 443;
			std::chrono::milliseconds transfer_time = std::chrono::duration::max; //обновляется при echo всегда. По желанию у всех адресов.
		};

		bool fails_echo = false;
		bool fails_detect = false;
		bool fails_fix = false;
		bool fails_ticket = false;
		std::string location = "";
		std::vector<Address> addresses;
	};

	std::set<NodeNum> invalid_node_numbers(const std::set<NodeID>& ids);
	std::set<NodeNum> invalid_node_numbers(const std::set<NodeNum>& nums);

	using NodeNum = unsigned short; //node number
	using NodeID = std::pair<NetID, NodeNum>;
	using ReqState = std::tuple<std::vector<NodeResponse>, ResultRep>;
	using Params = std::multimap<std::string, std::string>;
	using NodeAddresses = std::vector<NodeDirectory::Address>;

	class Service {
		struct Cache {
			DirDump directories;
		}

		CURLM* rhandle; //request handle
		Cache cache;

	protected:
		void init_libcurl();
		std::set<NodeID> filter_nodes(const std::set<NodeID>&);
		std::vector<std::vector<std::string>> request_addrs(
				NodeID node,
				const std::string& service,
				const Params& params,
				bool all_addrs)
		std::vector<NodeResponse> request(
				std::vector<NodeAddresses>&, const std::set<NodeID>& blacklist); 
		//Params detect_params(const CloudCoin& coin, NodeNum node, const std::string& pan);

		ticket

	public:
		Service(Cache);
		Service();

		void flush_bl();

		//ответы соответствуют элементам множества узлов
		ReqState detect(CloudCoin&, const std::set<NodeNum>&, bool ignore_bl = false);
		ReqState detect(CloudCoin&, bool ignore_bl = false);
		ReqState echo(
				const std::vector<NodeID>& nodes, 
				bool brief = false, 
				bool all_addrs = true);
		void directory();
		fix
	};

	std::set<NodeID> nodes(const DirDump& directories);


// ----- reading -----
	std::string json_object_get_string(json_t*, const std::string&);	
	ResultRep json_load_check(json_t* root, json_error_t* json_err)

	using ReadState = std::tuple<std::vector<CloudCoin>, ResultRep>;
	ReadState read_json(const std::string& path);
} //rot::cpc

#endif //CPC_H













/*


// ----- request -----	
	//for connection pool safe and other cache

		ReqState ticket(const CloudCoin& coin, NodeID node, std::set<NodeID>* const bl = nullptr);
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

	std::vector<std::string> get_url(
			const NodeID& node, 
			const std::string& service, 
			const Params& params);

	
		ReqState fix(CloudCoin& coin, std::set<NodeID>* const bl = nullptr);

	//expand content of raw request
	

// ----- other -----	
	
	std::vector<NodeResponse> wrap_raw_responses(const std::vector<Response>&);

*/
