/*
 * resume.c
 *
 * Handle resume from suspend-to-disk
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <linux/config.h>	/* For CONFIG_PM_STD_PARTITION */

#include "kinit.h"
#include "do_mounts.h"

#ifndef CONFIG_PM_STD_PARTITION
# define CONFIG_PM_STD_PARTITION ""
#endif

int do_resume(int argc, char *argv[])
{
	const char *resume_file = CONFIG_PM_STD_PARTITION;
	const char *resume_arg;
	dev_t resume_device;
	int powerfd = -1;
	char device_string[64];
	int len;

	resume_arg = get_arg(argc, argv, "resume=");
	resume_file = resume_arg ? resume_arg : resume_file;
	if (!resume_file[0])
		return 0;	/* No resume device specified */

	/* Fix: we either should consider reverting the device back to
	   ordinary swap, or (better) put that code into swapon */
	if (get_flag(argc, argv, "noresume") )
		return 0;	/* Noresume requested */

	resume_device = name_to_dev_t(resume_file);

	if (major(resume_device) == 0) {
		fprintf(stderr, "Invalid resume device: %s\n", resume_file);
		goto failure;
	}

	if ((powerfd = open("/sys/power/resume", O_WRONLY)) < 0)
		goto fail_r;

	len = snprintf(device_string, sizeof device_string, "%u:%u",
		       major(resume_device), minor(resume_device));

	if (len >= sizeof device_string)
		goto fail_r;	/* This should never happen */

	DEBUG(("kinit: trying to resume from %s\n", resume_file));

	if (write(powerfd, device_string, len) != len)
		goto fail_r;

	/* Okay, what are we still doing alive... */
failure:
	if (powerfd >= 0)
		close(powerfd);
	fprintf(stderr, "Resume failed, doing normal boot...\n");
	return -1;

fail_r:
	fprintf(stderr, "Resume failed: cannot write /sys/power/resume (no kernel support?)\n");
	goto failure;
}