#pragma once
#include <iostream>
#include <vector>
#include <dirent.h>
static void rt_assert(int cond, std::string err) {
	if (!cond) {
		std::cerr << err << "\n";
		exit(-1);
	}
}

static std::string ibdev2netdev(std::string ibdev_name) {
	std::string dev_dir = "/sys/class/infiniband/" + ibdev_name + "/device/net";

	std::vector<std::string> net_ifaces;
	DIR *dp;
	struct dirent *dirp;
	dp = opendir(dev_dir.c_str());
	rt_assert(dp != nullptr, "Failed to open directory " + dev_dir);

	while (true) {
		dirp = readdir(dp);
		if (dirp == nullptr) break;

		if (strcmp(dirp->d_name, ".") == 0) continue;
		if (strcmp(dirp->d_name, "..") == 0) continue;
		net_ifaces.push_back(std::string(dirp->d_name));
	}
	closedir(dp);

	rt_assert(net_ifaces.size() > 0, "Directory " + dev_dir + " is empty");
	return net_ifaces[0];
}
static int is_printable(char x) {
        return x != '\n' && x != '\r';
}
static void debug_buffer(char* buffer, size_t len) {
	for (int row = 0; row < (len + 15) / 16; row++) {
		for (int col = 0; col < 16; col++) {
			size_t p = row * 16 + col;
			if (p < len)
				printf("%02x ", (unsigned char) buffer[p]);
			else
				printf("   ");
		}
		printf(" ");
		for (int col = 0; col < 16; col++) {
			size_t p = row * 16 + col;
			if (p < len) {
				if (is_printable(buffer[p]))
					printf("%c", (unsigned char) buffer[p]);
				else
					printf(".");
			} else
				printf(" ");
		}
		printf("\n");
	}
}


static void memory_barrier() { asm volatile("" ::: "memory"); }
