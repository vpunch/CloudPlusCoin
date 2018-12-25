/*
 * 
 *
 * 
 *
 *
 */

#ifndef CPC_H
#define CPC_H

#include <array>
#include <regex>
#include <stdexcept>
#include <tuple>
#include <numeric>

//dependencies
#include <jansson.h>
#include <curl/curl.h>
#include <uuid/uuid.h>

//debug
#include <iostream>

#define CPC_NODE_COUNT 25 
#define CPC_DOMAIN_NAME_SIZE (256 + 1024)
#define CPC_TIMEOUT 1000
#define CPC_CONTIMEOUT 60
#define CPC_NODE_URL "https://raida%s.cloudcoin.global/service/%s"

namespace rot::cpc { //CloudPlusCoin. CloudCoin API

//
	using NodeID = std::pair<size_t, size_t>; //network, node 

	enum ResultCode {
		ok = 0b0,
		error = 0b1,
		parse_error = 0b10,
		init_error = 0b100,
		request_error = 0b1000
	};
	struct ResultReport {
		ResultCode code;
		// пустое или с разделителем \n, но без переноса в конце
		std::string expl = "";
	};
	class CPCError : public std::runtime_error {
		ResultCode code;
	public:
		explicit CPCError(ResultCode error_code, const std::string &what) : std::runtime_error(what) { code = error_code; }
		ResultCode get_code() const { return code; }
	};

	struct CCDate { //CloudCoin date
		unsigned short month = 0;
		unsigned short year = 0;

		std::string get_string() { return std::to_string(month) + '-' + std::to_string(year); }
		bool set_string(std::string date);
	};
	struct CloudCoin {
		std::array<std::string, CPC_NODE_COUNT>	authenticity_number;		//25 ANs
		unsigned int							serial_number;				//1-16,777,216
		unsigned short							network_number;				//1-256
		CCDate									expiration_date;
		char									pown[CPC_NODE_COUNT + 1];
		std::vector<std::string>				aoid;
	};

//
	std::tuple<std::vector<CloudCoin>, ResultReport> read_json(const std::string &path);

//
	struct Accumulator {
		char* data = NULL; // обязательно проинициализировать NULL'ом
		int size {0};
	};	
	size_t write_response(void* buffer, size_t size, size_t length, void* storage);
	void free_accumulators_data(Accumulator* requests, size_t rsize);
	struct Response {
		std::string content;
		ResultReport report;
	};
	std::tuple<std::vector<Response>, ResultReport> get_request(const std::vector<std::string> &urls);

	class NodeResponse {
		ResultReport report;
		std::string server;
		std::string status;
		std::string message;
		std::string time;
	public:
		NodeResponse() = delete;
		NodeResponse(const Response& raw);
		ResultReport get_report() const { return report; }
		std::string get_server() const { return server; }
		std::string get_status() const { return status; }
		std::string get_message() const { return message; }
		std::string get_time() const { return time; }
	};

// RAIDA service
	/*
	 *  To establish that RAIDA's nodes is ready to "Detect" operation
	 */
	std::tuple<std::vector<NodeResponse>, ResultReport> echo(bool brief = false);

	// Use cicle for detection many coins
	std::tuple<std::vector<NodeResponse>, ResultReport> detection(CloudCoin &coin);

//
	int coin_denomination(unsigned int sn);
	std::string guid();

} //rot::cpc

#endif //CPC_H
