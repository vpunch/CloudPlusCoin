#include "cpc.h"

namespace rot::cpc {
	bool CCDate::set_string(std::string date)
	{
		std::regex re {"^\\d{1,2}-\\d{4}$"};
		if (!std::regex_match(date, re)) 
			return false;

		std::istringstream dstream(date);
		dstream >> month;
		dstream.get();
		dstream >> year;

		return true;
	}

	ResultReport json_load_check(json_t* root, json_error_t* json_err)
	{
		std::ostringstream err_expl;

		if (!root) {
			err_expl << "On line " << json_err->line << ": " << json_err->text;
			return {parse_error, err_expl.str()};
		}
		if (!json_is_object(root)) {
			json_decref(root);
			return {parse_error, "root is not an object"};
		}

		return {ok, ""};
	}

	std::string json_object_get_string(json_t* root, const std::string& key)
	{
		json_t* value = json_object_get(root, key.c_str());
		if (json_is_string(value))
			return json_string_value(value);
		return "";
	}

	std::tuple<std::vector<CloudCoin>, ResultReport> read_json(const std::string &path)
	{
		std::vector<CloudCoin> cloudcoins;

		std::vector<std::string> warnings;

		json_error_t json_err;
        json_t* root = json_load_file(path.c_str(), 0, &json_err);
		ResultReport lchrep = json_load_check(root, &json_err); 
		if (lchrep.code)
			return {cloudcoins, lchrep};

		json_t* json_ccs = json_object_get(root, "cloudcoin");
		if (!json_is_array(json_ccs)) {
			json_decref(root);
			return {cloudcoins, ResultReport {parse_error, "cloudcoin is not an array"}};
		}

//start of coin
		for (size_t i = 0; i < json_array_size(json_ccs); ++i) {

			std::ostringstream warning;

			json_t* json_cc = json_array_get(json_ccs, i);
			if (!json_is_object(json_cc)) {
				warning << "Coin " << i << ": is not an object";
				warnings.push_back(warning.str());
				continue;
			}

			CloudCoin cloudcoin;

			json_t *ans, *aoid;

//network number
			int nn_num;
			std::string nn_str = json_object_get_string(json_cc, "nn");
			if (!nn_str.empty()) {
				nn_num = atoi(nn_str.c_str());	
			} else {
				warning << "Coin " << i << ": can't read nn";
				warnings.push_back(warning.str());
				continue;
			}
			if (nn_num < 1) {
				warning << "Coin " << i << ": invalid network number";
				warnings.push_back(warning.str());
				continue;
			}
			cloudcoin.network_number = (unsigned short)nn_num;
			
//serial_number
			int sn_num;
			std::string sn_str = json_object_get_string(json_cc, "sn");
			if (!sn_str.empty()) {
				sn_num = atoi(sn_str.c_str());	
			} else {
				warning << "Coin " << i << ": can't read sn";
				warnings.push_back(warning.str());
				continue;
			}
			if (sn_num < 1) {
				warning << "Coin " << i << ": invalid serial number number";
				warnings.push_back(warning.str());
				continue;
			}
			cloudcoin.serial_number = (unsigned short)sn_num;

//an
			ans = json_object_get(json_cc, "an");
			if (!json_is_array(ans)) {
				warning << "Coin " << i << ": an is not an array";
				warnings.push_back(warning.str());
				continue;
			}
			if (json_array_size(ans) != CPC_NODE_COUNT) {
				warning << "Coin " << i << ": not valid ANs number";
				warnings.push_back(warning.str());
				continue;
			}
			int j; 
			for (j = 0; j < CPC_NODE_COUNT; ++j) {
				json_t* an = json_array_get(ans, j);
				if (!json_is_string(an)) {
					warning << "Coin " << i << ": an " << j << " is not a string";
					warnings.push_back(warning.str());
					break;
				}
				const char* an_val = json_string_value(an);
				cloudcoin.authenticity_number[j] = an_val;
			}
			if (j != CPC_NODE_COUNT)
				continue;
			
//pown
			std::string pown_str = json_object_get_string(json_cc, "pown");
			if(pown_str.size() != CPC_NODE_COUNT)
				pown_str = "";
			strcpy(cloudcoin.pown, pown_str.c_str());

//ed
			std::string ed_str = json_object_get_string(json_cc, "ed");
			CCDate ed_val;
			ed_val.set_string(ed_str);
			cloudcoin.expiration_date = ed_val;

//aoid
			aoid = json_object_get(json_cc, "aoid");
			if (json_is_array(aoid)) {
				for (int j = 0; j < json_array_size(aoid); ++j) {
					json_t* aoid_element = json_array_get(aoid, j);
					if (!json_is_string(aoid_element))
						continue;
					cloudcoin.aoid.push_back(json_string_value(aoid_element));
				}
			}

			cloudcoins.push_back(cloudcoin);
		} //end of coin

		json_decref(root);

		std::string expl = "";
		if (warnings.size() > 0)
			//join warnings
			expl = std::accumulate(begin(warnings) + 1, end(warnings), warnings[0], 
					[](std::string t, std::string n) {
				return t += '\n' + n;
			});

		return {cloudcoins, ResultReport {ok, expl}};
	}

	void free_accumulators_data(Accumulator* accums, size_t rsize)
	{
		for (int i = 0; i < rsize; ++i) {
			free(accums[i].data);
			accums[i].data = NULL;
			accums[i].size = 0;
		}
	}

	std::tuple<std::vector<Response>, ResultReport> get_request(const std::vector<std::string> &urls)
	{	
// libcurl init
		std::vector<Response> responses;
		if (urls.empty()) {
			return {responses, {ok, ""}};
		}

		if (curl_global_init(CURL_GLOBAL_ALL))
			return {responses, {init_error, "Something went wrong while libcurl initialization"}};

		CURLM* mhandle = curl_multi_init();
		if (!mhandle)
			return {responses, {init_error, "Failure while multi handle curl initialization"}};

// var init
		responses.reserve(urls.size());

		std::vector<Accumulator> accums(urls.size());

		std::vector<CURL*> ehandles;
		ehandles.reserve(urls.size());

		char** errors = (char**) malloc(sizeof(char*) * urls.size());
		for (size_t i = 0; i < urls.size(); ++i)
			errors[i] = (char*) malloc(CURL_ERROR_SIZE);

// fill multi handle
		for (size_t i = 0; i < urls.size(); ++i) {
			CURL* ehandle = curl_easy_init();
			if (!ehandle) {
				responses.push_back({"", {init_error, "Failure while easy handle curl initialization"}});
				continue;
			}

			responses.emplace_back();

			curl_easy_setopt(ehandle, CURLOPT_URL, urls[i].c_str());
			curl_easy_setopt(ehandle, CURLOPT_WRITEFUNCTION, write_response);
			curl_easy_setopt(ehandle, CURLOPT_WRITEDATA, &accums[i]);
			curl_easy_setopt(ehandle, CURLOPT_ERRORBUFFER, errors[i]);
			curl_easy_setopt(ehandle, CURLOPT_CONNECTTIMEOUT, CPC_CONTIMEOUT);

			ehandles.push_back(ehandle);

			curl_multi_add_handle(mhandle, ehandle);
		}

// processing
		int running_handlers = 0;
		do {
			curl_multi_perform(mhandle, &running_handlers);
			curl_multi_wait(mhandle, NULL, 0, CPC_TIMEOUT, NULL);
		} while (running_handlers);

// read result
		for (int msgq; CURLMsg* msg = curl_multi_info_read(mhandle, &msgq);)
			if (msg->msg == CURLMSG_DONE) {

				size_t idx;
				for (idx = 0; idx < urls.size(); ++idx)
					if (msg->easy_handle == ehandles[idx])
						break;

				Response &response = responses[idx]; 
				if (accums[idx].data) {
					response.content = accums[idx].data; 
				}
				
				if (msg->data.result != CURLE_OK) {
					response.report.code = request_error;

					//size_t len = strlen(errors[idx]);
					response.report.expl = errors[idx];
				} else
					response.report = {ok, "Successfully done"}; 

				curl_multi_remove_handle(mhandle, msg->easy_handle);
				curl_easy_cleanup(msg->easy_handle);
			}

// free
		curl_multi_cleanup(mhandle);
		curl_global_cleanup();

		free_accumulators_data(accums.data(), accums.size());

		for (size_t i = 0; i < urls.size(); ++i)
			free(errors[i]);
		free(errors);

		return {responses, {ok, ""}};
	}

	size_t write_response(void* buffer, size_t size, size_t length, void* storage)
	{
		Accumulator* response = (Accumulator*) storage;
		int buffer_size = size * length;

		response->data = (char*) realloc(response->data, response->size + buffer_size + 1);
		memcpy(response->data + response->size, buffer, buffer_size);
		response->size += buffer_size;
		response->data[response->size] = 0;

		return buffer_size;
	}

	std::string suffix(NodeID node)
	{
		std::string r = std::to_string(node.second);
		//if (node.first > 1)
		//	r += "_Net" + std::to_string(node.first);
		return  r;
	}

	std::tuple<std::vector<NodeResponse>, ResultReport> echo(std::vector<NodeID> node, bool brief)
	{
		if (nodes.empty()) {
			nodes.reserve(CPC_NET_COUNT * CPC_NODE_COUNT);
			for (size_t i = 0; i < CPC_NET_COUNT; ++i)
				for(size_t j = 0; j < CPC_NODE_COUNT; ++j)
					nodes.emplace_back(i, j);
		}

		std::vector<std::string> urls;
		urls.reserve(nodes.size());

		std::string service = "echo";
		service += brief ? "?b=t" : "";

		for (size_t i = 0; i < nodes.size(); ++i) {
			char url[CPC_DOMAIN_NAME_SIZE];
			sprintf(
					url, 
					CPC_NODE_URL, 
					suffix({nodes[i].first, nodes[i].second}).c_str(), 
					service.c_str());
			urls.emplace_back(url);
		}

		auto [responses, report] = get_request(urls);
		std::vector<NodeResponse> result;

		if (!report.code) {
			result.reserve(nodes.size());
			for (size_t i = 0; i < nodes.size(); ++i) {
				result.emplace_back(responses[i]); //parameter for constructor here
			}
		}

		return {result, report};
	}

	int coin_denomination(unsigned int sn)
	{
		std::array<std::pair<unsigned short, unsigned int>, 6> delims {{
				{0, 1},
				{1, 2097153},
				{5, 4194305},
				{25, 6291457},
				{100, 14680065},
				{250, 16777217}
		}};
		for (auto it = begin(delims); it != end(delims); ++it)
			if (sn < it->second)
				return it->first;
		return 0;
	}

	std::string guid()
	{
		uuid_t uuid;
		uuid_generate_random ( uuid );
		char s[37];
		uuid_unparse ( uuid, s );	
		
		return std::regex_replace(s, std::regex {"-"}, "");
	}

	std::tuple<std::vector<NodeResponse>, ResultReport> detection(CloudCoin &coin)
	{
		std::string service = "detect";
		service += "?nn=" + std::to_string(coin.network_number);
		service += "&sn=" + std::to_string(coin.serial_number);
		service += "&denomination=" + std::to_string(coin_denomination(coin.serial_number));

		std::array<std::string, CPC_NODE_COUNT> pan;
		for (auto it = begin(pan); it != end(pan); ++it)
			*it = guid();

		std::vector<std::string> urls;
		urls.reserve(CPC_NODE_COUNT);
		for (int i = 0; i < CPC_NODE_COUNT; ++i) {
			std::cout << coin.authenticity_number[i] << std::endl;

			std::string url_suffix = service 
					+ "&an=" + coin.authenticity_number[i]
					+ "&pan=" + pan[i];

			char url[CPC_DOMAIN_NAME_SIZE];
			sprintf(url, CPC_NODE_URL, suffix({coin.network_number, i}).c_str(), url_suffix.c_str());
			std::cout << url << std::endl;
			urls.emplace_back(url);
		}

		auto [responses, report] = get_request(urls);
		std::vector<NodeResponse> result;

		if (!report.code) {	
			result.reserve(CPC_NODE_COUNT);
			for (size_t i = 0; i < CPC_NODE_COUNT; ++i) {
				result.emplace_back(responses[i]); //parameter for constructor here
			
				std::string status = result[i].get_status();
				if (status == "pass") {
					coin.authenticity_number[i] = pan[i];
					coin.pown[i] = status[0];
				} else if (status == "fail") {
					coin.pown[i] = status[0];
				} else if (status == "error") {
					coin.pown[i] = status[0];
				}
			}
		}

		return {result, report};
	}

	NodeResponse::NodeResponse(const Response &raw)
	{
		report = raw.report;

		if (report.code) {
			return;
		}

		json_error_t json_err;
		json_t* root = json_loads(raw.content.c_str(), 0, &json_err);
		ResultReport lchrep = json_load_check(root, &json_err); 
		if (lchrep.code) {
			message = raw.content;			
			return;
		}
		
		server = json_object_get_string(root, "server");
		status = json_object_get_string(root, "status");
		message = json_object_get_string(root, "message");
		time = json_object_get_string(root, "time");

		json_decref(root);
	}

} // rot::cpc
