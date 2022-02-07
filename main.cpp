// Copyright (C) 2020 Evan McBroom
#include <cxxopts.hpp>
#include <socks5.h>

int main(int argc, char** argv) {
	cxxopts::Options options("MicroSocks", "SOCKS5 Server");
	options.add_options()
		("1,auth-once", "Whitelist an ip address after authenticating to not requre a password on subsequent connections")
		("b,bind-address", "The address for the all forwarded connections", cxxopts::value<std::string>())
		("h,help", "Print this message")
		("l,listen-ip", "Ip address to listen on", cxxopts::value<std::string>()->default_value("0.0.0.0"))
		("p,port", "Port to listen on", cxxopts::value<std::string>()->default_value("1080"))
		("u,user", "The username to use for authentication", cxxopts::value<std::string>())
		("v,verbose", "Verbose output")
		("P,password", "The password to use for authentication", cxxopts::value<std::string>())
		;
	options.allow_unrecognised_options();
	auto result{ options.parse(argc, argv) };

	if (result.count("help")) {
		fprintf(stderr, "%s\n", options.help().c_str());
		exit(0);
	}

	bool auth{ result["1"].count() != 0 };
	bool user{ result["u"].count() != 0 };
	bool password{ result["p"].count() != 0 };

	if (user ^ password) {
		fprintf(stderr, "error: user and password must be used together\n");
		return 1;
	}

	if (auth && !user) {
		fprintf(stderr, "error: auth-once option must be used together with user/password\n");
		return 1;
	}

	Socks5Server server;
	server.setVerbose(result.count("1") != 0);

	if (user)
		server.setAuthentication(strdup(result["u"].as<std::string>().c_str()), strdup(result["P"].as<std::string>().c_str()), result.count("1") != 0);

	if (result.count("b"))
		server.setBindAddress(strdup(result["b"].as<std::string>().c_str()));

	for (size_t i = 1; i < argc; i++) {
		size_t c, l = strlen(argv[i]);
		for (i = 0; c < l; c++) argv[i][c] = 0;
	}

	if (!server.start(result["l"].as<std::string>().c_str(), atoi(result["p"].as<std::string>().c_str()))) {
		fprintf(stderr, "Socks5Server::start\n");
		return 1;
	}

	return 0;
}