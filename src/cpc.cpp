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

	ResultRep json_load_check(json_t* root, json_error_t* json_err)
	{
		std::ostringstream err_expl;

		if (!root) {
			err_expl << "On line " << json_err->line << ": " << json_err->text; //не содержит переноса в конце
			return {parse_error, err_expl.str()}; //str() не добавляет перенос
		}
		if (!json_is_object(root)) {
			json_decref(root);
			return {parse_error, "root is not an object"};
		}

		return {ok, ""};
	}

	std::string json_object_get_string(json_t* object, const char* key)
	{
		json_t* value = json_object_get(object, key);
		return (json_is_string(value)) ? json_string_value(value) : "";
	}

	ReadState read_json(const std::string& path)
	{
		std::vector<CloudCoin> cloudcoins;
		std::vector<std::string> warnings;

		json_error_t json_err;
        json_t* root = json_load_file(path.c_str(), 0, &json_err);
		ResultRep lch_rep = json_load_check(root, &json_err); 
		if (lch_rep.code)
			return {cloudcoins, lch_rep};

		json_t* json_ccs = json_object_get(root, "cloudcoin"); //json cloudcoins
		if (!json_is_array(json_ccs)) {
			json_decref(root);
			return {cloudcoins, {parse_error, "cloudcoin is not an array"}};
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
	
//network number
			NetID nn_num;

			json_t* json_nn = json_object_get(json_cc, "nn");
			if (json_is_string(json_nn)) {
				std::string nn_str = json_string_value(json_nn);
				//" -32hello" -> MAX-32
				//"hello32" -> std::invalid_argument
				nn_num = std::stoul(nn_str.c_str());	
			} else {
				warning << "Coin " << i << ": can't read nn";
				warnings.push_back(warning.str());
				continue;
			}

			//if (nn_num < 1) {
			//	warning << "Coin " << i << ": invalid network number";
			//	warnings.push_back(warning.str());
			//	continue;
			//}
			cloudcoin.nn = nn_num;
			
//serial_number
			SNType sn_num;

			json_t* json_sn = json_object_get(json_cc, "sn");
			if (json_is_string(json_sn)) {
				std::string sn_str = json_string_value(json_sn);
				sn_num = atoi(sn_str.c_str());
			} else {
				warning << "Coin " << i << ": can't read sn";
				warnings.push_back(warning.str());
				continue;
			}

			cloudcoin.sn = sn_num;

//an
			json_t* json_ans = json_object_get(json_cc, "an");
			if (!json_is_array(json_ans)) {
				warning << "Coin " << i << ": an is not an array";
				warnings.push_back(warning.str());
				continue;
			}
			if (json_array_size(json_ans) != CPC_NODE_COUNT) {
				warning << "Coin " << i << ": not valid ANs number";
				warnings.push_back(warning.str());
				continue;
			}
			int j; 
			for (j = 0; j < CPC_NODE_COUNT; ++j) {
				json_t* json_an = json_array_get(json_ans, j);
				if (!json_is_string(json_an)) {
					warning << "Coin " << i << ": an " << j << " is not a string";
					warnings.push_back(warning.str());
					break;
				}
				const char* an_val = json_string_value(json_an);
				cloudcoin.ans[j] = an_val;
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
			cloudcoin.ed = ed_val;

//aoid
			json_t* json_aoid = json_object_get(json_cc, "aoid");
			if (json_is_array(json_aoid)) {
				for (int j = 0; j < json_array_size(json_aoid); ++j) {
					json_t* json_aoid_el = json_array_get(json_aoid, j);
					if (!json_is_string(json_aoid_el))
						continue;
					cloudcoin.aoid.push_back(json_string_value(json_aoid_el));
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

		return {cloudcoins, {ok, expl}};
	}

	void free_accumulators(Accumulator* accums, size_t rsize)
	{
		for (int i = 0; i < rsize; ++i) {
			free(accums[i].data);
			accums[i].data = NULL;
			accums[i].size = 0;
		}
	}

	void init_libcurl()
	{
		//The environment it sets up is constant for the life of the program 
		//and is the same for every program, so multiple calls have the same effect as one call.
		static std::once_flag libcurl_inited;	
		std::call_once(libcurl_inited, [] {
			if (curl_global_init(CURL_GLOBAL_ALL))
				throw CPCError(init_error, "Something went wrong while libcurl initialization");

			std::atexit([] {
				curl_global_cleanup();
			});
		});

		static std::once_flag rh_inited;
		std::call_once(rh_inited, [] {
			rhandle = curl_multi_init();
			if (!rhandle)
				throw CPCError(init_error, "Failure while multi handle curl initialization");

			/* only keep 10 connections in the cache */
			curl_multi_setopt(rhandle, CURLMOPT_MAXCONNECTS, 10L);

			std::atexit([] {
				curl_multi_cleanup(rhandle);			
			});
		});
	}

	//количество url равно количеству ответов, если report ok
	RawReqState request(const std::vector<std::string>& urls)
	{	
		std::vector<Response> responses;
		if (urls.empty()) {
			return {responses, {ok, ""}};
		}

// init
		try {
			init_libcurl();	
		} catch (CPCError error) {
			return {responses, {error.get_code(), error.what()}};
		}

		responses.reserve(urls.size());

		std::vector<Accumulator> accums(urls.size());

		std::vector<CURL*> ehandles;
		ehandles.reserve(urls.size());

		char** errors = (char**) malloc(sizeof(char*) * urls.size());
		for (size_t i = 0; i < urls.size(); ++i)
			errors[i] = (char*) malloc(CURL_ERROR_SIZE);

// sync
		static std::mutex rmtx; //request mutex
		std::unique_lock<std::mutex> locker(rmtx);

// fill multi handle
		for (size_t i = 0; i < urls.size(); ++i) {
			CURL* ehandle = curl_easy_init();

			ehandles.push_back(ehandle);

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
			curl_easy_setopt(ehandle, CURLOPT_TIMEOUT, CPC_TFRTIMEOUT);

			curl_multi_add_handle(rhandle, ehandle);
		}

// processing
		int running_handlers = 0;
		do {
			curl_multi_perform(rhandle, &running_handlers);
			curl_multi_wait(rhandle, NULL, 0, CPC_TIMEOUT, NULL);
		} while (running_handlers);

// read result
		for (int msgq; CURLMsg* msg = curl_multi_info_read(rhandle, &msgq);)
			if (msg->msg == CURLMSG_DONE) {

				size_t idx;
				for (idx = 0; idx < urls.size(); ++idx)
					if (msg->easy_handle == ehandles[idx])
						break;
				Response& response = responses[idx]; 

				if (accums[idx].data) {
					response.content = accums[idx].data; 
				}
				
				if (msg->data.result != CURLE_OK) {
					response.report.code = request_error;

					//size_t len = strlen(errors[idx]);
					response.report.expl = errors[idx];
				} else
					response.report = {ok, "Successfully done"};

				curl_multi_remove_handle(rhandle, msg->easy_handle);
				curl_easy_cleanup(msg->easy_handle);
			}

// free
		free_accumulators(accums.data(), accums.size());

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

	std::string get_url(
			const NodeID& node, 
			const std::string& service, 
			const Params& params)
	{
		char url[CPC_DOMAIN_NAME_SIZE];
		std::string tail = service + (params.empty() ? "" : "?");

		bool is_multi = false;
		for (auto it = params.begin(); it != params.end(); ++it) {
			auto nit = std::next(it);
			if (!is_multi && it->first == nit->first)
				is_multi = true;

			tail += 
				it->first + 
				((is_multi) ? "[]=" : "=") + 
				it->second + 
				(nit != params.end() ? "&" : "");

			if (is_multi && it->first != nit->first)
				is_multi = false;
		}

		sprintf(
				url,
				CPC_NODE_URL,
				std::to_string(node.second).c_str(),
				tail.c_str());

		//std::cout << url << std::endl;
		return url;
	}

	std::vector<NodeResponse> wrap_raw_responses(const std::vector<Response>& rresps)
	{
		std::vector<NodeResponse> result;
		result.reserve(rresps.size());
		for (rresp : rresps)
			result.emplace_back(rresp);
		return result;
	}

	ReqState echo(const std::set<NodeID>& nodes, bool brief)
	{
		if (nodes.empty()) {
			return {std::vector<NodeResponse>, {ok, ""}};
		}

		std::vector<std::string> urls;
		urls.reserve(nodes.size());
		for (auto node : nodes)
			urls.push_back(
					get_url(node, "echo", brief ? Params {{"b", "t"}} : Params {}));

		auto [responses, report] = request(urls);
		assert(nodes.size() == responses.size());

		return {wrap_raw_responses(responses), report};
	}

	int coin_denomination(SNType sn)
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

	ReqState detect(CloudCoin& coin, const std::set<NodeID>* bl)
	{	
		/*
		auto [resps, rep] = prev_try;

		if (resps.size() > 0 && resps.size() != CPC_NODE_COUNT) {
			rep.code = parse_error;
			rep.expl = "Invalid responses count for previous detect operation";
			return;
		}
		*/

		std::vector<std::string> urls;
		urls.reserve(CPC_NODE_COUNT);
		for (size_t i = 0; i < CPC_NODE_COUNT; ++i) {
			//if (resps.
			Params params {
				{"nn", std::to_string(coin.nn)},
				{"sn", std::to_string(coin.sn)},
				{"denomination", std::to_string(coin_denomination(coin.sn))},
				{"an", coin.ans[i]},
				{"pan", pan[i]}};
			urls.push_back(get_url({coin.nn, i}, "detect", params));
		}

		std::array<std::string, CPC_NODE_COUNT> pan;
		for (auto& e : pan)
			e = guid();	

		

		auto [raw_responses, report] = request(urls);
		assert(raw_responses.size() == CPC_NODE_COUNT);

		auto responses = wrap_raw_responses(raw_responses)

		if (!report.code)
			for (size_t i = 0; i < CPC_NODE_COUNT; ++i) {
				const std::string& status = responses[i].get_status();
				if (status == "pass") {
					coin.ans[i] = pan[i];
					coin.pown[i] = status[0];
				} else if (status == "fail") {
					coin.pown[i] = status[0];
				} else if (status == "error") {
					coin.pown[i] = status[0];
				}
			}

		return {responses, report};
	}

	NodeResponse::NodeResponse()
	{
		report.code = undefined;
	}

	NodeResponse::NodeResponse(const Response& raw)
	{
		report = raw.report;
		if (report.code) {
			return;
		}

		json_error_t json_err;
		json_t* root = json_loads(raw.content.c_str(), 0, &json_err);
		ResultRep lch_rep = json_load_check(root, &json_err); 
		if (lch_rep.code) {
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
