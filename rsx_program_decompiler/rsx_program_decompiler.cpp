#include <rsx/rsx.h>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <functional>

template<typename DecompilerType>
int process(const std::string& path)
{
	if (auto &file_stream = std::ifstream{ path, std::ios::binary })
	{
		u32 buffer[512 * 4];
		u32 size = file_stream.read((char*)buffer, sizeof(buffer)).gcount();
		auto info = DecompilerType{ buffer, size }.decompile();
		std::cout << info.text;
		return 0;
	}

	return -3;
}

const std::unordered_map<std::string, std::function<int(const std::string&)>> g_profiles =
{
	{ "fp_glsl", process<rsx::fragment_program::glsl_decompiler> },
	//{ "vp_glsl", process<rsx::vertex_program::glsl_decompiler> }
};

void help()
{
	std::cout << "usage: [profile] <path to ucode>" << std::endl;
	std::cout << "supported profiles: ";
	for (auto &profile : g_profiles)
	{
		std::cout << profile.first << " ";
	}
	std::cout << std::endl;
}

int main(int argc, char** argv)
{
	if (argc != 3)
	{
		help();
		return -1;
	}

	auto found = g_profiles.find(std::string(argv[1]).substr(1));

	if (found == g_profiles.end())
	{
		help();
		return -2;
	}

	return found->second(argv[2]);
}

