/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstring>
#include <cstdlib>
#include <ctime>
#include <iostream>

#include <libudev.h>
#include <signal.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <fmt/core.h>
#include <fmt/color.h>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/printf.h>

#define PROC_NAME "usb-mon"
#define PROC_VERSION "1.0"
#define MAX_EVENTS 5

static bool global_exit = false;

//
// Signal Callback
//
void signalsHandler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM)
	{
		fmt::print(stdout, fg(fmt::color::red) | fmt::emphasis::bold,"\n** Signal Triggered **");
		fmt::print(stdout, "\n");
		global_exit = true;
	}
}

static void printDevice(struct udev_device *device, const char *source, int prop)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	printf("%-6s[%llu.%06u] %-8s %s (%s)\n",
		   source,
		   (unsigned long long)ts.tv_sec, (unsigned int)ts.tv_nsec / 1000,
		   udev_device_get_action(device),
		   udev_device_get_devpath(device),
		   udev_device_get_subsystem(device));
	if (prop)
	{
		struct udev_list_entry *list_entry;

		udev_list_entry_foreach(list_entry, udev_device_get_properties_list_entry(device))
			printf("%s=%s\n",
				   udev_list_entry_get_name(list_entry),
				   udev_list_entry_get_value(list_entry));
		printf("\n");
	}
}

static void printDevice(struct udev_device *dev)
{
	const char *action = udev_device_get_action(dev);
	if (!action)
		action = "exists";

	const char *vendor = udev_device_get_sysattr_value(dev, "idVendor");
	if (!vendor)
		vendor = "0000";

	const char *product = udev_device_get_sysattr_value(dev, "idProduct");
	if (!product)
		product = "0000";
	
	const char *devnum = udev_device_get_sysattr_value(dev, "devnum");
	if (!devnum)
		devnum = "";
    const char *busnum = udev_device_get_sysattr_value(dev, "busnum");
	if (!busnum)
		busnum = "";

	const char *manufacturer = udev_device_get_sysattr_value(dev, "manufacturer");
	if (!manufacturer)
		manufacturer = "";

	const char *dev_path = udev_device_get_devnode(dev);
	if (!dev_path)
		dev_path = "---";

	fmt::print(stdout, "{:12} {:15} {:8} {:4} {:4} {:4}:{:7} {:40} {:30}\n",
		   udev_device_get_subsystem(dev),
		   udev_device_get_devtype(dev),
		   action,
		   devnum,
		   busnum,
		   vendor,
		   product,
		   manufacturer,
		   dev_path);
}

static void processDevice(struct udev_device *dev)
{
	if (dev)
	{
		if (udev_device_get_devnode(dev))
			printDevice(dev);

		udev_device_unref(dev);
	}
}

static void monitorDevices(struct udev *udev)
{
	int fd_ep = -1;
	int fd_udev = -1;
	struct epoll_event ep_udev;
	struct udev_monitor *udev_monitor;

	fd_ep = epoll_create1(EPOLL_CLOEXEC);
	if (fd_ep < 0)
	{
		fmt::print(stderr, "error creating epoll fd: %m\n");
		return;
	}

	//
	// Create Link udev Monitor events 'udev'
	//
	udev_monitor = udev_monitor_new_from_netlink(udev, "udev");
	if (udev_monitor == nullptr)
	{
		fmt::print(stderr, "error: unable to create netlink socket\n");
		if (fd_ep >= 0)
        	close(fd_ep);
		return;
	}

	//
	// Set size buffer udev_monitor
	//
	udev_monitor_set_receive_buffer_size(udev_monitor, 128 * 1024 * 1024);

	//
	// Create file descriptor udev_monitor
	//
	fd_udev = udev_monitor_get_fd(udev_monitor);
	if (fd_udev <= 0)
	{
		fmt::print(stderr, "error: udev_monitor_get_fd");
		udev_monitor_unref(udev_monitor);
		if (fd_ep >= 0)
        	close(fd_ep);
		return;
	}

	if (udev_monitor_filter_add_match_subsystem_devtype(udev_monitor, "usb", nullptr) < 0)
	{
		fmt::print(stderr, "error: unable to apply subsystem filter 'usb'\n");
		udev_monitor_unref(udev_monitor);
		if (fd_ep >= 0)
        	close(fd_ep);
		return;
	}

	//
	// Check enable receiving events udev
	//
	if (udev_monitor_enable_receiving(udev_monitor) < 0)
	{
		fmt::print(stderr, "error: unable to subscribe to udev events\n");
		udev_monitor_unref(udev_monitor);
		if (fd_ep >= 0)
        	close(fd_ep);
		return;
	}

	//
	// Set epoll FD udev
	//
	ep_udev.events = EPOLLIN | EPOLLET;
	ep_udev.data.fd = fd_udev;
	if (epoll_ctl(fd_ep, EPOLL_CTL_ADD, fd_udev, &ep_udev) < 0)
	{
		fmt::print(stderr, "Fail to add fd to epoll\n");
		udev_monitor_unref(udev_monitor);
		if (fd_ep >= 0)
        	close(fd_ep);
		return;
	}

	fmt::print(stdout, fmt::emphasis::bold, "\nStarted USB Monitor\n");
	fmt::print(stdout, "\n{:12} {:15} {:8} {:4} {:4} {:12} {:40} {:30}\n", 
	                   "SUBSYSTEM", "TYPE", "ACTION", "DEV", "BUS", "VID:PID", 
					   "MANUFACTURER", "PATH");
	while (!global_exit)
	{
		int fdcount;
		struct epoll_event ev[4];
		int i;

		fdcount = epoll_wait(fd_ep, ev, MAX_EVENTS, -1);
		if (fdcount < 0)
		{
			if (errno != EINTR)
			{
				fmt::print(stderr, "error receiving uevent message: %m\n");
			}
			continue;
		}

		for (i = 0; i < fdcount; i++)
		{
			if (ev[i].data.fd == fd_udev && ev[i].events & EPOLLIN)
			{
				struct udev_device *device;

				device = udev_monitor_receive_device(udev_monitor);
				if (device == nullptr)
					continue;

				// printDevice(device, "UDEV", true);
				printDevice(device);
				
				udev_device_unref(device);
			}
		}
	}

	if (fd_ep >= 0)
		close(fd_ep);
}

static void enumerateDevices(struct udev *udev)
{
	struct udev_enumerate *enumerate = udev_enumerate_new(udev);

	udev_enumerate_add_match_subsystem(enumerate, "usb");
	udev_enumerate_scan_devices(enumerate);

	struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
	struct udev_list_entry *entry;

	fmt::print(stdout, fmt::emphasis::bold, "\nListing USB Devices\n");
	fmt::print(stdout, "\n{:12} {:15} {:8} {:4} {:4} {:12} {:40} {:30}\n", 
	                   "SUBSYSTEM", "TYPE", "ACTION", "DEV", "BUS", "VID:PID", 
					   "MANUFACTURER", "PATH");
	udev_list_entry_foreach(entry, devices)
	{
		const char *path = udev_list_entry_get_name(entry);
		struct udev_device *dev = udev_device_new_from_syspath(udev, path);
		processDevice(dev);
	}

	udev_enumerate_unref(enumerate);
}

int main()
{
	struct udev *udev;
	struct sigaction act;
	sigset_t mask;

	fmt::print(stdout, "Name     : {}\n", PROC_NAME);
	fmt::print(stdout, "Version  : {}\n", PROC_VERSION);
	fmt::print(stdout, "PID:     : {}\n", getpid());
	//
	// Register Date Time Started Application
	//
	std::time_t dt = std::time(nullptr);
	fmt::print(stdout, "Started  : {:%Y-%m-%d %H:%M:%S}\n", fmt::localtime(dt));

	//
	// Set Signal Handlers
	//
	std::memset(&act, 0x00, sizeof(struct sigaction));
	act.sa_handler = signalsHandler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_RESTART;
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGTERM);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);

 	//
	// Create udev object
	//
	udev = udev_new();
	if (!udev)
	{
		fmt::print(stderr, "udev_new() failed\n");
		return 1;
	}

	enumerateDevices(udev);

	monitorDevices(udev);

	//
	// Free Object udev
	//
	udev_unref(udev);

	//
	// Register Date Time Finished Application
	//
	dt = std::time(nullptr);
	fmt::print(stdout, "Finished : {:%Y-%m-%d %H:%M:%S}\n", fmt::localtime(dt));
	
	return EXIT_SUCCESS;
}
